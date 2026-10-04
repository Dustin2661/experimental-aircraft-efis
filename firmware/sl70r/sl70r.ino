#include "Sl70r.h"
#include <stdio.h>
#include <string.h>

// Bench-only opt-in: checksum and IDENT behavior still need verification.
#ifndef SL70R_ENABLE_BENCH_TX
#define SL70R_ENABLE_BENCH_TX 0
#endif

efis::Sl70r transponder;
char hostCommand[32];
size_t hostLength = 0;
bool hostOverflow = false;
uint32_t hostLastByte = 0;
uint32_t lastRelay = 0;

void reply(const char* text) {
    const size_t length = strlen(text);
    if (Serial && Serial.availableForWrite() >= static_cast<int>(length)) {
        Serial.write(reinterpret_cast<const uint8_t*>(text), length);
    }
}

void handleCommand(uint32_t now) {
    if (!SL70R_ENABLE_BENCH_TX) {
        reply("{\"command\":\"rejected\",\"reason\":\"bench_tx_disabled\"}\n");
        return;
    }
    if (Serial3.availableForWrite() < static_cast<int>(efis::Sl70r::CommandLength)) {
        reply("{\"command\":\"rejected\",\"reason\":\"uart_busy\"}\n");
        return;
    }
    char packet[16];
    bool accepted = false;
    if (strncmp(hostCommand, "SQUAWK ", 7) == 0) {
        accepted = transponder.setSquawk(hostCommand + 7, now, packet);
    } else if (strlen(hostCommand) == 6 && strncmp(hostCommand, "MODE ", 5) == 0) {
        accepted = transponder.setMode(hostCommand[5], now, packet);
    } else if (strcmp(hostCommand, "IDENT") == 0) {
        accepted = transponder.ident(now, packet);
    } else if (strcmp(hostCommand, "EMERGENCY") == 0) {
        accepted = transponder.emergency(now, packet);
    } else if (strncmp(hostCommand, "RELEASE ", 8) == 0) {
        accepted = transponder.releaseEmergency(hostCommand + 8, now, packet);
    }
    if (accepted) {
        Serial3.write(reinterpret_cast<const uint8_t*>(packet),
                      efis::Sl70r::CommandLength);
        reply("{\"command\":\"sent_unconfirmed\"}\n");
    } else {
        reply("{\"command\":\"rejected\"}\n");
    }
}

void serviceHost(uint32_t now) {
    if ((hostLength != 0 || hostOverflow) && now - hostLastByte >= 1000) {
        hostLength = 0;
        hostOverflow = false;
    }
    // Limit USB work so a busy host cannot starve the transponder UART.
    for (size_t budget = 0; budget < 64 && Serial.available() > 0; ++budget) {
        const char byte = static_cast<char>(Serial.read());
        hostLastByte = now;
        if (byte == '\r') {
            continue;
        }
        if (byte == '\n') {
            if (!hostOverflow && hostLength != 0) {
                hostCommand[hostLength] = '\0';
                handleCommand(now);
            }
            hostLength = 0;
            hostOverflow = false;
        } else if (byte < 32 || byte > 126 || hostLength >= sizeof(hostCommand) - 1) {
            hostOverflow = true;
        } else if (!hostOverflow) {
            hostCommand[hostLength++] = byte;
        }
    }
}

void relayStatus(uint32_t now) {
    if (now - lastRelay < 1000) return;
    lastRelay = now;
    const auto& state = transponder.status();
    char line[384];
    const int length = snprintf(
        line, sizeof(line),
        "{\"device\":\"sl70r\",\"received\":%s,\"fresh\":%s,"
        "\"mode\":\"%c\",\"squawk\":\"%s\",\"ident\":%s,"
        "\"heartbeat_received\":%s,\"replies_fresh\":%s,\"replies_per_second\":%u,"
        "\"pending\":%s,\"command_timeout\":%s,\"emergency_latched\":%s,"
        "\"invalid_frames\":%lu,\"bench_tx_enabled\":%s}\n",
        state.received ? "true" : "false",
        transponder.fresh(now) ? "true" : "false",
        state.mode, state.squawk, state.ident ? "true" : "false",
        (state.flags & 1) != 0 ? "true" : "false",
        transponder.repliesFresh(now) ? "true" : "false",
        static_cast<unsigned int>(state.repliesPerSecond),
        transponder.pending() ? "true" : "false",
        transponder.commandTimedOut() ? "true" : "false",
        transponder.emergencyLatched() ? "true" : "false",
        static_cast<unsigned long>(transponder.invalidFrames()),
        SL70R_ENABLE_BENCH_TX ? "true" : "false");
    if (length > 0 && static_cast<size_t>(length) < sizeof(line)) {
        reply(line);
    }
}

void setup() {
    Serial.begin(115200);
    Serial3.begin(9600, SERIAL_8N1);
}

void loop() {
    const uint32_t now = millis();
    transponder.tick(now);
    for (size_t budget = 0; budget < 128 && Serial3.available() > 0; ++budget) {
        transponder.feed(static_cast<char>(Serial3.read()), now);
    }
    serviceHost(now);
    relayStatus(now);
}
