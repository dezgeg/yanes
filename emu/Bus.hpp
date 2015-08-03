#pragma once

#include "Irq.hpp"
#include "Logger.hpp"
#include "Platform.hpp"
#include "Rom.hpp"
#include "Serializer.hpp"
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

    IrqSet irqsPending;
    bool lastNmiState;
    unsigned spriteDmaBytes;
    Byte spriteDmaBank;

    Byte ram[2048];

    void memAccess(Word address, Byte* pData, bool isWrite, MemAccessType accessType);
public:
    Bus(Logger* log, Rom* rom, Gpu* gpu, Joypad* joypad, Sound* sound) :
            log(log),
            rom(rom),
            gpu(gpu),
            joypad(joypad),
            sound(sound),
            irqsPending(0),
            lastNmiState(false),
            spriteDmaBytes(0),
            spriteDmaBank(0) {
        std::memset(ram, 0xAA, sizeof(ram));
    }

    Rom* getRom() { return rom; }
    void serialize(Serializer& ser);

    Byte memRead8(Word address, MemAccessType accessType = "CPU");
    void memWrite8(Word address, Byte value, MemAccessType accessType = "CPU");
    Word memRead16(Word address, MemAccessType accessType = "CPU");
    void memWrite16(Word address, Word value, MemAccessType accessType = "CPU");

    void setNmiState(bool state);
    IrqSet getPendingIrqs();
    bool tickDma();
};
