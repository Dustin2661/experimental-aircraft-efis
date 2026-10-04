#ifndef EFIS_SL70R_H
#define EFIS_SL70R_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace efis {

class Sl70r {
public:
    static constexpr uint32_t StatusTimeoutMs = 3500;
    static constexpr uint32_t FrameTimeoutMs = 150;
    static constexpr uint32_t CommandTimeoutMs = 3500;
    static constexpr size_t CommandLength = 15;

    struct Status {
        bool received = false;
        char mode = 'O';
        char squawk[5] = "0000";
        bool ident = false;
        uint8_t flags = 0;
        uint32_t updatedAt = 0;
        bool repliesReceived = false;
        uint16_t repliesPerSecond = 0;
        uint32_t repliesUpdatedAt = 0;
    };

    const Status& status() const { return status_; }
    bool pending() const { return pending_; }
    bool commandTimedOut() const { return commandTimedOut_; }
    bool emergencyLatched() const { return emergencyLatched_; }
    uint32_t invalidFrames() const { return invalidFrames_; }

    bool fresh(uint32_t now) const {
        return status_.received && elapsed(now, status_.updatedAt) < StatusTimeoutMs;
    }

    bool repliesFresh(uint32_t now) const {
        return status_.repliesReceived &&
               elapsed(now, status_.repliesUpdatedAt) < StatusTimeoutMs;
    }

    void tick(uint32_t now) {
        if (length_ != 0 && elapsed(now, lastByteAt_) >= FrameTimeoutMs) {
            length_ = 0;
            ++invalidFrames_;
        }
        if (pending_ && elapsed(now, requestedAt_) >= CommandTimeoutMs) {
            pending_ = false;
            commandTimedOut_ = true;
        }
    }

    void feed(char byte, uint32_t now) {
        tick(now);
        if (byte == '^') {
            if (length_ != 0) {
                ++invalidFrames_;
            }
            length_ = 0;
            frame_[length_++] = byte;
            lastByteAt_ = now;
            return;
        }
        if (length_ == 0) {
            return;
        }
        lastByteAt_ = now;
        if (byte == '\r') {
            parse(now);
            length_ = 0;
        } else if (length_ < sizeof(frame_)) {
            frame_[length_++] = byte;
        } else {
            length_ = 0;
            ++invalidFrames_;
        }
    }

    static bool validSquawk(const char* value) {
        if (value == nullptr) {
            return false;
        }
        for (size_t i = 0; i < 4; ++i) {
            if (value[i] < '0' || value[i] > '7') {
                return false;
            }
        }
        return value[4] == '\0';
    }

    // Additive checksum is inferred from manual examples, not yet OEM-verified.
    static uint8_t checksum(const char* bytes, size_t count) {
        uint8_t result = 0;
        for (size_t i = 0; i < count; ++i) {
            result = static_cast<uint8_t>(result + static_cast<uint8_t>(bytes[i]));
        }
        return result;
    }

    bool setSquawk(const char* value, uint32_t now, char (&output)[16]) {
        if (emergencyLatched_ || !validSquawk(value) ||
            strcmp(value, "7500") == 0 || strcmp(value, "7600") == 0 ||
            strcmp(value, "7700") == 0) {
            return false;
        }
        return prepare(status_.mode, value, false, false, now, output);
    }

    bool setMode(char mode, uint32_t now, char (&output)[16]) {
        return prepare(mode, status_.squawk, false, false, now, output);
    }

    bool ident(uint32_t now, char (&output)[16]) {
        if (status_.mode == 'O' || status_.ident) {
            return false;
        }
        return prepare(status_.mode, status_.squawk, true, false, now, output);
    }

    bool emergency(uint32_t now, char (&output)[16]) {
        if (emergencyLatched_) {
            return false;
        }
        if (!prepare(status_.mode, "7700", false, false, now, output)) {
            return false;
        }
        emergencyLatched_ = true;
        return true;
    }

    bool releaseEmergency(const char* value, uint32_t now, char (&output)[16]) {
        if (!emergencyLatched_ || !validSquawk(value) ||
            strcmp(value, "7500") == 0 || strcmp(value, "7600") == 0 ||
            strcmp(value, "7700") == 0) {
            return false;
        }
        return prepare(status_.mode, value, false, true, now, output);
    }

private:
    Status status_;
    char frame_[16] = {};
    size_t length_ = 0;
    uint32_t lastByteAt_ = 0;
    uint32_t invalidFrames_ = 0;
    bool pending_ = false;
    bool commandTimedOut_ = false;
    bool emergencyLatched_ = false;
    bool releasePending_ = false;
    char requestedMode_ = 'O';
    char requestedSquawk_[5] = {};
    bool requestedIdent_ = false;
    uint32_t requestedAt_ = 0;

    static uint32_t elapsed(uint32_t now, uint32_t then) {
        return now - then;
    }

    static bool validMode(char mode) {
        return mode == 'O' || mode == 'A' || mode == 'C';
    }

    static int hexValue(char value) {
        if (value >= '0' && value <= '9') return value - '0';
        if (value >= 'A' && value <= 'F') return value - 'A' + 10;
        if (value >= 'a' && value <= 'f') return value - 'a' + 10;
        return -1;
    }

    static int hexByte(const char* bytes) {
        const int high = hexValue(bytes[0]);
        const int low = hexValue(bytes[1]);
        return high < 0 || low < 0 ? -1 : high * 16 + low;
    }

    bool prepare(char mode, const char* squawk, bool ident, bool release,
                 uint32_t now, char (&output)[16]) {
        tick(now);
        if (!fresh(now) || pending_ || !validMode(mode) || !validSquawk(squawk)) {
            return false;
        }
        memcpy(output, "#MD ", 4);
        output[4] = mode;
        output[5] = ',';
        output[6] = ident ? 'I' : '-';
        output[7] = ',';
        memcpy(output + 8, squawk, 4);
        const uint8_t sum = checksum(output, 12);
        const char* hex = "0123456789ABCDEF";
        output[12] = hex[sum >> 4];
        output[13] = hex[sum & 15];
        output[14] = '\r';
        output[15] = '\0';
        requestedMode_ = mode;
        memcpy(requestedSquawk_, squawk, 5);
        requestedIdent_ = ident;
        releasePending_ = release;
        requestedAt_ = now;
        pending_ = true;
        commandTimedOut_ = false;
        return true;
    }

    void parse(uint32_t now) {
        if (length_ == 16 && memcmp(frame_, "^MD ", 4) == 0) {
            char squawk[5] = {};
            memcpy(squawk, frame_ + 8, 4);
            const int flags = hexByte(frame_ + 12);
            if (!validMode(frame_[4]) || frame_[5] != ',' ||
                (frame_[6] != 'I' && frame_[6] != '-') || frame_[7] != ',' ||
                !validSquawk(squawk) || flags < 0 ||
                hexByte(frame_ + 14) != checksum(frame_, 14)) {
                ++invalidFrames_;
                return;
            }
            status_.received = true;
            status_.mode = frame_[4];
            status_.ident = frame_[6] == 'I';
            memcpy(status_.squawk, squawk, 5);
            status_.flags = static_cast<uint8_t>(flags);
            status_.updatedAt = now;
            if (strcmp(squawk, "7700") == 0) {
                emergencyLatched_ = true;
            }
            if (pending_ && status_.mode == requestedMode_ &&
                strcmp(squawk, requestedSquawk_) == 0 &&
                (!requestedIdent_ || status_.ident)) {
                pending_ = false;
                if (releasePending_) emergencyLatched_ = false;
            }
        } else if (length_ == 10 && memcmp(frame_, "^RC ", 4) == 0) {
            uint16_t count = 0;
            for (size_t i = 4; i < 8; ++i) {
                if (frame_[i] < '0' || frame_[i] > '9') {
                    ++invalidFrames_;
                    return;
                }
                count = static_cast<uint16_t>(count * 10 + frame_[i] - '0');
            }
            if (hexByte(frame_ + 8) != checksum(frame_, 8)) {
                ++invalidFrames_;
                return;
            }
            status_.repliesReceived = true;
            status_.repliesPerSecond = count;
            status_.repliesUpdatedAt = now;
        }
        // Other SL70R messages are deliberately unsupported, not misparsed.
    }
};

} // namespace efis

#endif
