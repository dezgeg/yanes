#pragma once

typedef const char* MemAccessType; // Just in case we want an enum instead someday...

union Regs;
class Bus;
class Logger
{
    bool insnLoggingEnabled;

    long currentFrame;
    long currentCycle;
    int currentScanline;

public:
    Logger(bool insnLoggingEnabled=false) :
        insnLoggingEnabled(insnLoggingEnabled)
    {
    }

    void setTimestamp(long frame, int scanline, long cycle)
    {
        currentFrame = frame;
        currentCycle = cycle;
        currentScanline = scanline;
    }

    void logInsn(Bus* bus, Regs* regs, int cycles, Word newPC, const char* fmt, ...);
    void logMemoryAccess(Word addr, Byte data, bool isWrite, MemAccessType accessType);
    void warn(const char* fmt, ...);
};
