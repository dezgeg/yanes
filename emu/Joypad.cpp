#include "Joypad.hpp"
#include "Logger.hpp"

void Joypad::regAccess(Word address, Byte* pData, bool isWrite) {
    unsigned port = address - 0x4016;
    if (port == 0 && isWrite) {
        resetLatch = !!(*pData & 1);
//        log->warn("Joypad: Set reset latch: %d", resetLatch != 0);
    }
    if (resetLatch)
        latchedKeys = latestKeys;

    if (!isWrite) {
        if (port == 0) {
            *pData = (latchedKeys & 0x1) | 0x40;
            latchedKeys = (latchedKeys >> 1) | 0x80;
//            log->warn("Joypad: Returned keys to NES: %02x (reset latch = %d, state = %02x)",
//                      (unsigned)*pData, (int)resetLatch, latestKeys);
        } else {
            *pData = 0x40; // not connected
        }
    }
}
