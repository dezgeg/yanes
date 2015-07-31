#pragma once

#include "Platform.hpp"

enum PadKeys {
    Pad_A = 1 << 0,
    Pad_B = 1 << 1,
    Pad_Select = 1 << 2,
    Pad_Start = 1 << 3,
    Pad_Up = 1 << 4,
    Pad_Down = 1 << 5,
    Pad_Left = 1 << 6,
    Pad_Right = 1 << 7,

    Pad_AllKeys = 0xff,
};

class Joypad {
    Logger* log;
    Byte currentKeys;   // state of keys pressed in GUI, 1 = pressed
    Byte latestKeys;    // latest keys delivered to emulation core
    Byte latchedKeys;
    bool resetLatch;

public:
    Joypad(Logger* log) :
            log(log),
            currentKeys(0),
            latestKeys(0),
            latchedKeys(0),
            resetLatch(false) {
    }

    bool tick() {
        latestKeys = currentKeys;
        return false;
    }

    void regAccess(Word address, Byte* pData, bool isWrite) {
        unsigned port = address - 0x4016;
        if (port == 0 && isWrite) {
            resetLatch = !!(*pData & 1);
        }
        if (resetLatch)
            latchedKeys = latestKeys;

        if (!isWrite) {
            if (port == 0) {
                *pData = (latchedKeys & 0x1) | 0x40;
                latchedKeys = (latchedKeys << 1) | 0x80;
                log->warn("Returned keys to NES: %02x", (unsigned)*pData);
            } else {
                *pData = 0x2; // not connected
            }
        }
    }

    void keysPressed(Byte keys) {
        currentKeys |= keys;
    }

    void keysReleased(Byte keys) {
        currentKeys &= ~keys;
    }
};
