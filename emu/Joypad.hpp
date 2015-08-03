#pragma once

#include "Platform.hpp"
#include "Serializer.hpp"

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

class Logger;

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

    void regAccess(Word address, Byte* pData, bool isWrite);

    void keysPressed(Byte keys) {
        currentKeys |= keys;
    }

    void keysReleased(Byte keys) {
        currentKeys &= ~keys;
    }

    void serialize(Serializer& ser) {
        ser.handleObject("Joypad.currentKeys", currentKeys);
        ser.handleObject("Joypad.latestKeys", latestKeys);
        ser.handleObject("Joypad.latchedKeys", latchedKeys);
        ser.handleObject("Joypad.resetLatch", resetLatch);
    }
};
