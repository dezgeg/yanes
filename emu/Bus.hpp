#pragma once

#include "Irq.hpp"
#include "Logger.hpp"
#include "Platform.hpp"
#include "Rom.hpp"
#include "Gpu.hpp"
#include "Joypad.hpp"
#include "Sound.hpp"

#include <cstring>

class Gpu;

class Joypad;

class Rom;

class Sound;

class Bus {
    Logger* log;
    Rom* rom;
    Gpu* gpu;
    Joypad* joypad;
    Sound* sound;

    IrqSet irqsEnabled;
    IrqSet irqsPending;

    Byte ram[2048];

    void memAccess(Word address, Byte* pData, bool isWrite, MemAccessType accessType);
public:
    Bus(Logger* log, Rom* rom, Gpu* gpu, Joypad* joypad, Sound* sound) :
            log(log),
            rom(rom),
            gpu(gpu),
            joypad(joypad),
            sound(sound),
            irqsEnabled(0),
            irqsPending(0) {
        std::memset(ram, 0xAA, sizeof(ram));
    }

    Rom* getRom() { return rom; }

    Byte memRead8(Word address, MemAccessType accessType = "CPU");
    void memWrite8(Word address, Byte value, MemAccessType accessType = "CPU");
    Word memRead16(Word address, MemAccessType accessType = "CPU");
    void memWrite16(Word address, Word value, MemAccessType accessType = "CPU");

    void raiseIrq(IrqSet irqs);
    void ackIrq(Irq irq);
    IrqSet getEnabledIrqs();
    IrqSet getPendingIrqs();
};
