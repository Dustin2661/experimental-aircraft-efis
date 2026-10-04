#include "../firmware/sl70r/Sl70r.h"
#include <assert.h>
#include <stdio.h>
#include <string>

using efis::Sl70r;

static void feed(Sl70r& device, const char* message, uint32_t now) {
    while (*message) device.feed(*message++, now);
}

static std::string mode(char m, const char* code, bool ident = false,
                        const char* flags = "01") {
    char message[18];
    snprintf(message, sizeof(message), "^MD %c,%c,%s%s", m, ident ? 'I' : '-', code, flags);
    const uint8_t sum = Sl70r::checksum(message, 14);
    snprintf(message + 14, 4, "%02X\r", sum);
    return message;
}

int main() {
    Sl70r device;
    char output[16];
    assert(!device.setSquawk("1200", 0, output));
    assert(!device.fresh(0));
    assert(Sl70r::validSquawk("0000"));
    assert(Sl70r::validSquawk("7777"));
    assert(!Sl70r::validSquawk(nullptr));
    for (const char* bad : {"", "123", "12345", "1280", "-100", "12a0"}) {
        assert(!Sl70r::validSquawk(bad));
    }
    feed(device, "^MD O,-,12000006\r", 10);
    assert(device.fresh(10));
    assert(device.status().mode == 'O');
    assert(strcmp(device.status().squawk, "1200") == 0);
    assert(device.status().flags == 0);
    assert(!device.ident(10, output));
    assert(device.setMode('A', 11, output));
    assert(device.pending());
    assert(!device.setSquawk("2345", 12, output));
    feed(device, mode('A', "1200").c_str(), 20);
    assert(!device.pending());
    assert(device.ident(21, output));
    assert(strcmp(output, "#MD A,I,120079\r") == 0);
    feed(device, mode('A', "1200").c_str(), 22);
    assert(device.pending());
    feed(device, "^MD A,I,23540120\r", 23);
    assert(device.pending());
    feed(device, mode('A', "1200", true).c_str(), 24);
    assert(!device.pending());
    assert(!device.ident(25, output));
    feed(device, "^RC 1200D6\r", 30);
    assert(device.status().repliesPerSecond == 1200);
    assert(device.repliesFresh(30));
    feed(device, "^MD A,-,12000100\r", 31);
    assert(device.status().updatedAt == 24);
    assert(device.invalidFrames() == 1);
    feed(device, "^RC 12x0D6\r", 32);
    assert(device.status().repliesUpdatedAt == 30);
    feed(device, "^MD A,-,12800100\r", 33);
    assert(device.status().updatedAt == 24);
    feed(device, mode('C', "0000", false, "03").c_str(), 40);
    assert(device.status().flags == 3);
    assert(device.setSquawk("0001", 41, output));
    device.tick(41 + Sl70r::CommandTimeoutMs);
    assert(!device.pending());
    assert(device.commandTimedOut());
    assert(!device.fresh(41 + Sl70r::CommandTimeoutMs));
    assert(!device.setMode('A', 4000, output));
    feed(device, mode('C', "0000").c_str(), 4001);
    assert(device.emergency(4002, output));
    assert(device.emergencyLatched());
    feed(device, mode('C', "7700").c_str(), 4003);
    assert(!device.pending());
    assert(!device.setSquawk("1200", 4004, output));
    assert(!device.releaseEmergency("7700", 4004, output));
    assert(device.releaseEmergency("1200", 4004, output));
    assert(device.emergencyLatched());
    device.tick(4004 + Sl70r::CommandTimeoutMs);
    assert(device.emergencyLatched());
    feed(device, mode('C', "7700").c_str(), 8000);
    assert(device.releaseEmergency("1200", 8001, output));
    feed(device, mode('C', "1200").c_str(), 8002);
    assert(!device.emergencyLatched());
    assert(!device.setSquawk("7500", 8003, output));
    assert(!device.setSquawk("7600", 8003, output));
    assert(!device.setSquawk("7700", 8003, output));
    assert(!device.setMode('X', 8003, output));

    Sl70r framing;
    feed(framing, "noise^MD A,", 0);
    framing.tick(Sl70r::FrameTimeoutMs);
    assert(framing.invalidFrames() == 1);
    feed(framing, "I,23540120\r", 151);
    assert(!framing.status().received);
    feed(framing, "^xxxxxxxxxxxxxxxxxxxxxxxx\r", 152);
    assert(framing.invalidFrames() == 2);
    feed(framing, "^MD ^MD O,-,12000006\r\n", 153);
    assert(framing.status().received);
    assert(framing.invalidFrames() == 3);
    feed(framing, "^RC 1200D6\r", 154);
    feed(framing, "^RC 1200D6\r", 4000);
    assert(!framing.fresh(4000));
    assert(framing.repliesFresh(4000));

    Sl70r wrap;
    feed(wrap, mode('C', "1200").c_str(), UINT32_MAX - 100);
    assert(wrap.fresh(100));
    assert(wrap.setMode('A', UINT32_MAX - 50, output));
    wrap.tick(100);
    assert(wrap.pending());
    wrap.tick(3500);
    assert(wrap.commandTimedOut());

    Sl70r examples;
    feed(examples, mode('O', "1200").c_str(), 0);
    assert(examples.setMode('O', 1, output));
    assert(strcmp(output, "#MD O,-,12006B\r") == 0);
    feed(examples, mode('A', "2354").c_str(), 2);
    examples.tick(3501);
    feed(examples, mode('A', "2354").c_str(), 3502);
    assert(examples.ident(3503, output));
    assert(strcmp(output, "#MD A,I,235484\r") == 0);

    for (unsigned int number = 0; number < 4096; ++number) {
        Sl70r codes;
        char code[5];
        snprintf(code, sizeof(code), "%04o", number);
        feed(codes, mode('C', code).c_str(), 0);
        assert(codes.status().received);
        assert(strcmp(codes.status().squawk, code) == 0);
    }
    puts("SL70R tests passed");
}
