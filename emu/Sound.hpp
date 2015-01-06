#pragma once

#include "BusUtil.hpp"
#include "Logger.hpp"
#include "Platform.hpp"


struct SoundRegs {
};

class Sound {
    Logger* log;
    SoundRegs regs;

    unsigned long currentCycle;
    long cycleResidue;
    long currentSampleNumber;

    uint16_t leftSample;
    uint16_t rightSample;

public:
    Sound(Logger* log);

    void generateSamples();
    void registerAccess(Word address, Byte* pData, bool isWrite);
    void tick(int cycleDelta);

    long getCurrentSampleNumber() { return currentSampleNumber; }
    uint16_t getLeftSample() { return leftSample; }
    uint16_t getRightSample() { return rightSample; }
};
