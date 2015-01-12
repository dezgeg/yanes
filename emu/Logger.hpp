#pragma once

typedef const char* MemAccessType; // Just in case we want an enum instead someday...

struct Regs;

class Bus;

enum LogFlags {
    Log_Insns = 1,
    Log_MemoryAccesses = 2,
};

class Logger {
    long currentFrame;
    long currentCycle;
    int currentScanline;

protected:
    virtual void logImpl(const char* format, ...) = 0;

public:
    LogFlags logFlags;

    Logger() :
            logFlags() {
    }

    void setTimestamp(long frame, int scanline, long cycle) {
        currentFrame = frame;
        currentCycle = cycle;
        currentScanline = scanline;
    }

    void logInsn(Bus* bus, Regs* regs, int cycles, Word newPC, const char* fmt, ...);
    void logMemoryAccess(Word addr, Byte data, bool isWrite, MemAccessType accessType);
    void logDebug(const char* fmt, ...);
    void warn(const char* fmt, ...);
};
