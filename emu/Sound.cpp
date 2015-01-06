#include <string.h>
#include <QDebug>
#include "Sound.hpp"
#include "Utils.hpp"

#include <bitset>

void Sound::registerAccess(Word address, Byte* pData, bool isWrite) {
}

void Sound::tick(int cycleDelta) {
}

void Sound::generateSamples() {
}

Sound::Sound(Logger* log) : log(log),
                            currentCycle(),
                            cycleResidue(),
                            currentSampleNumber(),
                            leftSample(),
                            rightSample() {
    memset(&regs, 0, sizeof(regs));
}
