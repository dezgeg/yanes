#pragma once

#include "Bus.hpp"
#include "Logger.hpp"
#include "Platform.hpp"
#include "Serializer.hpp"

struct Regs {
    Byte a;
    Byte x, y;
    Byte sp;
    union {
        Word pc;
        struct {
            Byte pcLo;
            Byte pcHi;
        };
    };
    union Flags {
        struct {
            bool c : 1;
            bool z : 1;
            bool i : 1;
            bool d : 1;
            bool b : 1;
            bool v : 1;
            bool n : 1;
        };
        Byte bits;
    } flags;

    void setNZ(Byte b) {
        flags.z = b == 0;
        flags.n = !!(b & 0x80);
    }
};

#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
# error "You are using piss-poor hardware and are a loser; give up and get a real job."
#endif

class Nes;

class Cpu {
    Logger* log;
    Bus* bus;
    Regs regs;

    Byte getPcByte() {
        Byte ret = bus->memRead8(regs.pc);
        regs.pc++;
        return ret;
    }

    Word getPcWord() {
        Word ret = bus->memRead16(regs.pc);
        regs.pc += 2;
        return ret;
    }

    void push(Byte b) {
        bus->memWrite8(0x100 | regs.sp, b);
        regs.sp--;
    }

    Byte pull() {
        regs.sp++;
        return bus->memRead8(0x100 | regs.sp);
    }

    Word pullWord() {
        Byte lo = pull();
        Byte hi = pull();
        return (hi << 8) | lo;
    }

    int pageCrossCycles(Word addr, int diff) {
        return (addr & 0xff00) != ((addr + diff) & 0xff00);
    }

    Byte doAddSub(Byte lhs, Byte rhs, bool isAdd, bool isCmp);

    long doTick(Byte opcode);
    long handleColumn4C(Byte nybble, bool b);
    long handleColumnA(Byte i);
    long handleColumn8(Byte nybble);
    long handleColumn0(Byte nybble);
    long handleColumn6E(Byte opcode);
    long handleColumn159D(Byte opcode);

public:
    Cpu(Logger* log, Bus* bus) :
            log(log),
            bus(bus) {
        reset();
    }

    Regs* getRegs() { return &regs; }

    void reset();
    long tick();
    void serialize(Serializer& ser);
};
