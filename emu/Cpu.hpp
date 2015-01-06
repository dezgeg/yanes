#pragma once

#include "Bus.hpp"
#include "Logger.hpp"
#include "Platform.hpp"

union Regs {
    Word pc;
};

#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
# error "You are using piss-poor hardware and are a loser; give up and get a real job."
#endif

class Nes;

class Cpu {
    Logger* log;
    Bus* bus;
    Regs regs;

public:
    Cpu(Logger* log, Bus* bus) :
            log(log),
            bus(bus) {
        reset();
    }

    Regs* getRegs() { return &regs; }

    void reset();
    long tick();
};
