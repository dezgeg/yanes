#include "Cpu.hpp"

#include <stdio.h>

#ifndef CONFIG_NO_INSN_TRACE
#define INSN_DBG(x) x
#define INSN_DBG_DECL() bool _branched = false; Word branchPc = 0; Regs _savedRegs = regs; _savedRegs.pc -= 1
#define INSN_BRANCH(newPc) (_branched = true, branchPc = regs.pc, regs.pc = (newPc))
#define INSN_DONE(cycles, ...) (log->logInsn(bus, &_savedRegs, cycles, _branched ? branchPc : regs.pc, __VA_ARGS__), cycles)
#else
#define INSN_DBG(x)
#define INSN_DBG_DECL()
#define INSN_BRANCH(newPc) regs.pc = (newPc)
#define INSN_DONE(cycles, ...) cycles
#endif

Byte Cpu::doAddSub(Byte lhs, Byte rhs, bool isAdd, bool isCmp) {
    SByte carry = isCmp ? 1 : regs.flags.c;

    Byte result;
    if (isAdd) {
        result = lhs + rhs + carry;
        regs.flags.c = (Word(lhs) + Word(rhs) + carry) > WORD_MAX;
    } else {
        result = lhs - rhs - Byte(1 - carry);
        regs.flags.c = !((Word(lhs) + Word(rhs) + carry) > WORD_MAX);
    }

    bool lhsSign = !!(lhs & 0x80);
    bool rhsSign = !!(rhs & 0x80);
    bool resultSign = !!(result & 0x80);
    regs.flags.v = lhsSign == rhsSign && resultSign != lhsSign;
    regs.setNZ(result);

    return result;
}

static const Word VECTOR_BRK = 0xfffe;

void Cpu::reset() {
    regs = Regs();
}

long Cpu::tick() {
    Byte opcode = getPcByte();
    long cycles = doTick(opcode);
    if (!cycles) {
        INSN_DBG_DECL();
        log->warn("Unknown instruction, emulated as NOP");
        return INSN_DONE(1, "UNK #0x%02x", opcode);
    }
    return cycles;
}

long Cpu::doTick(Byte opcode) {
    INSN_DBG_DECL();

    // Hard-code certain ones: column x2, JMPs, holes
    switch (opcode) {
        case 0xa2: {
            regs.x = getPcByte();
            regs.setNZ(regs.x);
            return INSN_DONE(2, "LDX #0x%02x", regs.x);
        }
        case 0x4c: {
            Word addr = getPcWord();
            regs.pc = addr;
            return INSN_DONE(3, "JMP $0x%04x", addr);
        }
        case 0x6c: {
            Word indAddr = getPcWord();
            // TODO: this instruction should be buggy?
            regs.pc = bus->memRead16(indAddr);
            return INSN_DONE(3, "JMP ($0x%04x)", indAddr);
        }

        case 0x89:
        case 0x9c:
        case 0x9e:
            return 0;
    }

    Byte lowNybble = opcode & 0xf;
    Byte highNybble = opcode >> 4;
    switch (lowNybble) {
        case 0x0:
            return handleColumn0(highNybble);

        case 0x1:
        case 0x5:
        case 0x9:
        case 0xd:
            return handleColumn159D(opcode);

        case 0x4:
        case 0xc:
            return handleColumn4C(highNybble, lowNybble == 0xc);

        case 0x6:
        case 0xe:
            return handleColumn6E(opcode);

        case 0x8:
            return handleColumn8(highNybble);

        case 0xa:
            return handleColumnA(highNybble);
    }

    return 0;
}

#define DO_BRANCH(cond, opc) { \
        SByte displacement = getPcByte(); \
        Word oldPc = regs.pc; \
        Word dest = regs.pc + displacement; \
        if (cond) \
            regs.pc = dest; \
        return INSN_DONE(cond ? 3 + pageCrossCycles(oldPc, displacement) : 2, opc " $0x%04x", dest); \
    }

long Cpu::handleColumn0(Byte highNybble) {
    INSN_DBG_DECL();

    switch (highNybble) {
        case 0x0: {
            regs.flags.b = 1;
            push(regs.flags.bits);
            regs.flags.b = 0;

            // No regs.pc--; since BRK has an unused byte operand
            // XXX(RTS): off by one?
            push(regs.pcHi);
            push(regs.pcLo);
            regs.flags.i = 1;
            regs.pc = bus->memRead16(VECTOR_BRK);
            return INSN_DONE(7, "BRK");
        }

        case 0x1: DO_BRANCH(regs.flags.n == 0, "BPL");

        case 0x2: {
            Word addr = getPcWord();
            regs.pc--; // XXX(RTS): off by one?
            push(regs.pcHi);
            push(regs.pcLo);
            regs.pc = addr;
            return INSN_DONE(6, "JSR $0x%04x", addr);
        }

        case 0x3: DO_BRANCH(regs.flags.n == 1, "BMI");

        case 0x4: {
            regs.pcLo = pull();
            regs.pcHi = pull();
            regs.pc++; // XXX(RTS) off by one?

            regs.flags.bits = pull();
            regs.flags.b = 0;
            return INSN_DONE(6, "RTI");
        }

        case 0x5: DO_BRANCH(regs.flags.v == 0, "BVC");

        case 0x6: {
            regs.pcLo = pull();
            regs.pcHi = pull();
            regs.pc++; // XXX(RTS) off by one?
            return INSN_DONE(6, "RTS");
        }

        case 0x7: DO_BRANCH(regs.flags.v == 1, "BVS");
        case 0x9: DO_BRANCH(regs.flags.c == 0, "BCC");

        case 0xa: {
            regs.y = getPcByte();
            regs.setNZ(regs.y);
            return INSN_DONE(2, "LDY #0x%02x", regs.y);
        }
    }

    return 0;
}

long Cpu::handleColumn4C(Byte highNybble, bool isTwoByte) {
    INSN_DBG_DECL();
    Word addr = isTwoByte ? getPcWord() : getPcByte();
    Word mask = isTwoByte ? 0xffff : 0xff;
    int fieldWidth = isTwoByte ? 4 : 2;

    switch (highNybble) {
        case 0x2: {
            regs.setNZV(bus->memRead8(addr));
            return INSN_DONE(3, "BIT $0x%0*x", fieldWidth, addr);
        }
        case 0x8: {
            bus->memWrite8(addr, regs.y);
            return INSN_DONE(3, "STY $0x%0*x", fieldWidth, addr);
        }
        case 0x9: {
            bus->memWrite8((addr + regs.x) & mask, regs.y);
            return INSN_DONE(4, "STY $0x%0*x, x", fieldWidth, addr);
        }
        case 0xa: {
            regs.y = bus->memRead8(addr);
            regs.setNZ(regs.y);
            return INSN_DONE(3, "LDY $0x%0*x", fieldWidth, addr);
        }
        case 0xb: {
            regs.y = bus->memRead8((addr + regs.x) & mask);
            regs.setNZ(regs.y);
            return INSN_DONE(4 + pageCrossCycles(addr, regs.x), "LDY $0x%0*x, x", fieldWidth, addr);
        }
        case 0xc: {
            doAddSub(regs.y, bus->memRead8(addr), false, true);
            return INSN_DONE(3, "CPY $0x%0*x", fieldWidth, addr);
        }
        case 0xe: {
            doAddSub(regs.x, bus->memRead8(addr), false, true);
            return INSN_DONE(3, "CPX $0x%0*x", fieldWidth, addr);
        }
    }
    return 0;
}

long Cpu::handleColumn8(Byte highNybble) {
    INSN_DBG_DECL();
    switch (highNybble) {
        case 0x0:
            push(regs.flags.bits);
            return INSN_DONE(3, "PHP");

        case 0x1:
            regs.flags.c = 0;
            return INSN_DONE(2, "CLC");

        case 0x2:
            regs.flags.bits = pull();
            regs.flags.b = 0;
            return INSN_DONE(4, "PLP");

        case 0x3:
            regs.flags.c = 1;
            return INSN_DONE(2, "SEC");

        case 0x4:
            push(regs.a);
            return INSN_DONE(3, "PHA");

        case 0x5:
            regs.flags.i = 0;
            return INSN_DONE(2, "CLI");

        case 0x6:
            regs.a = pull();
            return INSN_DONE(4, "PLA");

        case 0x7:
            regs.flags.i = 1;
            return INSN_DONE(2, "SEI");

        case 0x8:
            regs.y--;
            regs.setNZ(regs.y);
            return INSN_DONE(2, "DEY");

        case 0x9:
            regs.a = regs.y;
            return INSN_DONE(2, "TYA");

        case 0xa:
            regs.y = regs.a;
            return INSN_DONE(2, "TAY");

        case 0xb:
            regs.flags.v = 0;
            return INSN_DONE(2, "CLV");

        case 0xc:
            regs.y++;
            regs.setNZ(regs.y);
            return INSN_DONE(2, "INY");

        case 0xd:
            regs.flags.d = 0;
            return INSN_DONE(2, "CLD");

        case 0xe:
            regs.x++;
            regs.setNZ(regs.x);
            return INSN_DONE(2, "INX");

        case 0xf:
            regs.flags.d = 1;
            return INSN_DONE(2, "SED");
    }
    return 0;
}

long Cpu::handleColumnA(Byte highNybble) {
    INSN_DBG_DECL();

    switch (highNybble) {
        case 0x8:
            regs.a = regs.x;
            return INSN_DONE(2, "TXA");

        case 0x9:
            regs.sp = regs.x;
            return INSN_DONE(2, "TXS");

        case 0xa:
            regs.x = regs.a;
            return INSN_DONE(2, "TAX");

        case 0xb:
            regs.x = regs.sp;
            return INSN_DONE(2, "TSX");

        case 0xc:
            regs.x--;
            regs.setNZ(regs.x);
            return INSN_DONE(2, "DEX");

        case 0xe:
            return INSN_DONE(2, "NOP");
    }
    return 0;
}

long Cpu::handleColumn6E(Byte opcode) {
    INSN_DBG_DECL();
    // insn bytes: 06, 0e, 16, 1e
    // addr modes: zp, abs, zpx, abx
    // therefore: bit 3 = zp/abs, bit 4 = no xy/yes xy
    bool isTwoByte = !!(opcode & bit(3));
    bool hasX = !!(opcode & bit(4));

    Word addr = isTwoByte ? getPcWord() : getPcByte();
    Word mask = isTwoByte ? 0xffff : 0xff;
    int fieldWidth = isTwoByte ? 4 : 2;

    // XXX: instruction 0xBE should have page crossing cost!
    // XXX: LDXes should use Y as index register!
    Word effAddr = hasX ? addr + regs.x : addr;

    const char* str;
    Byte b = bus->memRead8(effAddr & mask);
    switch ((opcode >> 4) & ~1) {
        case 0x0:
            regs.flags.c = !!(b & 0x80);
            b <<= 1;
            regs.setNZ(b);
            str = "ASL";
            break;

        case 0x2:
            regs.flags.c = !!(b & 0x80);
            b = (b << 1) | (b >> 7);
            regs.setNZ(b);
            str = "ROL";
            break;

        case 0x4:
            regs.flags.c = !!(b & 0x80);
            b >>= 1;
            regs.setNZ(b);
            str = "LSR";
            break;

        case 0x6:
            regs.flags.c = !!(b & 0x80);
            b = (b >> 1) | (b << 7);
            regs.setNZ(b);
            str = "ROR";
            break;

        case 0x8:
            // XXX don't read, index reg is Y
            bus->memWrite8(effAddr, regs.x);
            str = "STX";
            break;

        case 0xa:
            // XXX don't write, index reg is Y
            regs.x = b;
            regs.setNZ(b);
            str = "LDX";
            break;

        case 0xc:
            b--;
            regs.setNZ(b);
            str = "DEC";
            break;

        case 0xe:
            b++;
            regs.setNZ(b);
            str = "INC";
            break;
    }
    bus->memWrite8(effAddr, b);
    // XXX cycle count is wrong
    return INSN_DONE(6 + hasX, "%s $0x$*x$s", str, fieldWidth, addr, hasX ? ", X" : "");
}

long Cpu::handleColumn159D(Byte opcode) {
    INSN_DBG_DECL();
    // insn bytes: 01, 05, 09, 0D, 11, 15, 19, 1D
    // addr modes: izx, zp, imm, abs, izy, zpx, aby, abx
    int amode = (opcode >> 2) & 0x7;

    const char* fmt;
    Byte b;
    Word addrVal;
    switch (amode) {
        case 0:
            fmt = "%s ($0x%02x, X)";
            addrVal = getPcByte();
            b = bus->memRead8(bus->memRead16((addrVal + regs.x) & 0xff));
            break;

        case 1:
            fmt = "%s $0x%02x";
            addrVal = getPcByte();
            b = bus->memRead8(addrVal);
            break;

        case 2:
            fmt = "%s #0x%02x";
            addrVal = b = getPcByte();
            break;

        case 3:
            fmt = "%s 0x%04x";
            addrVal = getPcWord();
            b = bus->memRead8(addrVal);
            break;

        case 4:
            fmt = "%s (0x%02x), Y";
            addrVal = getPcByte();
            b = bus->memRead8(bus->memRead16(addrVal) + regs.y);
            break;

        case 5:
            fmt = "%s 0x%02x, X";
            addrVal = getPcByte();
            b = bus->memRead8((addrVal + regs.x) & 0xff);
            break;

        case 6:
            fmt = "%s 0x%04x, Y";
            addrVal = getPcWord();
            b = bus->memRead8(addrVal + regs.y);
            break;

        case 7:
            fmt = "%s 0x%04x, X";
            addrVal = getPcWord();
            b = bus->memRead8(addrVal + regs.x);
            break;
    }

    const char* str;
    switch ((opcode >> 4) & ~1) {
        case 0x0:
            regs.a |= b;
            regs.setNZ(regs.a);
            str = "ORA";
            break;

        case 0x2:
            regs.a &= b;
            regs.setNZ(regs.a);
            str = "AND";
            break;

        case 0x4:
            regs.a ^= b;
            regs.setNZ(regs.a);
            str = "EOR";
            break;

        case 0x6:
            regs.a = doAddSub(regs.a, b, true, false);
            str = "ADC";
            break;

        case 0x8:
            // XXX no load
            str = "STA";
            break;

        case 0xa:
            regs.a = b;
            regs.setNZ(regs.a);
            str = "LDA";
            break;

        case 0xc:
            doAddSub(regs.a, b, false, true);
            str = "CMP";
            break;

        case 0xe:
            regs.a = doAddSub(regs.a, b, false, false);
            str = "SBC";
            break;
    }

    // XXX cycle count
    return INSN_DONE(42, fmt, str, addrVal);
}
