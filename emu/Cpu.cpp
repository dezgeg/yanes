#include "Cpu.hpp"

#include <stdio.h>

#ifndef CONFIG_NO_INSN_TRACE
#define INSN_DBG(x) x
#define INSN_DBG_DECL() bool _branched = false; Word _branchPc = 0; Regs _savedRegs = regs; _savedRegs.pc -= 1
#define INSN_BRANCH(newPc) (_branched = true, _branchPc = regs.pc, regs.pc = (newPc))
#define INSN_DONE(cycles, ...) (log->logInsn(bus, &_savedRegs, cycles, _branched ? _branchPc : regs.pc, __VA_ARGS__), cycles)
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
        regs.flags.c = (Word(lhs) + Word(rhs) + carry) > 0xff;
    } else {
        result = lhs - rhs - Byte(1 - carry);
        regs.flags.c = !(SWord(lhs) - SWord(rhs) - SWord(1 - carry) < 0);
    }

    bool lhsSign = !!(lhs & 0x80);
    bool rhsSign = !!(rhs & 0x80);
    bool resultSign = !!(result & 0x80);

    regs.setNZ(result);
    if (!isCmp) {
        regs.flags.v = lhsSign == rhsSign && resultSign != lhsSign;
    }

    return result;
}

static const Word VECTOR_NMI = 0xfffa;
static const Word VECTOR_RST = 0xfffc;
static const Word VECTOR_BRK = 0xfffe;
static const Word VECTOR_IRQ = 0xfffe;

void Cpu::reset() {
    regs = Regs();
    regs.sp = 0xfd;
    regs.pc = bus->memRead16(VECTOR_RST);
}

long Cpu::tick() {
    IrqSet pendingIrqs = bus->getPendingIrqs();
    bool nmiPending = pendingIrqs & Irq_NMI;
    if (nmiPending || (!regs.flags.i && pendingIrqs & Irq_IRQ)) {
        log->warn("IRQ hit!");
        // XXX: don't copypaste
        push(regs.flags.bits);

        // XXX(RTS): off by one?
        regs.pc--;
        push(regs.pcHi);
        push(regs.pcLo);
        regs.flags.i = 1;
        regs.pc = bus->memRead16(nmiPending ? VECTOR_NMI : VECTOR_IRQ);
        return 4; // XXX IRQ latency?
    }

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
            INSN_BRANCH(addr);
            return INSN_DONE(3, "JMP $0x%04x", addr);
        }
        case 0x6c: {
            Word indAddr = getPcWord();
            // TODO: this instruction should be buggy?
            INSN_BRANCH(bus->memRead16(indAddr));
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
            INSN_BRANCH(dest); \
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
            INSN_BRANCH(bus->memRead16(VECTOR_BRK));
            return INSN_DONE(7, "BRK");
        }

        case 0x1: DO_BRANCH(regs.flags.n == 0, "BPL");

        case 0x2: {
            Word addr = getPcWord();
            regs.pc--; // XXX(RTS): off by one?
            push(regs.pcHi);
            push(regs.pcLo);
            regs.pc++;
            INSN_BRANCH(addr);
            return INSN_DONE(6, "JSR $0x%04x", addr);
        }

        case 0x3: DO_BRANCH(regs.flags.n == 1, "BMI");

        case 0x4: {
            INSN_BRANCH(pullWord() + 1); // XXX(RTS) off by one?

            regs.flags.bits = pull();
            regs.flags.b = 0;
            return INSN_DONE(6, "RTI");
        }

        case 0x5: DO_BRANCH(regs.flags.v == 0, "BVC");

        case 0x6: {
            INSN_BRANCH(pullWord() + 1); // XXX(RTS) off by one?
            return INSN_DONE(6, "RTS");
        }

        case 0x7: DO_BRANCH(regs.flags.v == 1, "BVS");
        case 0x9: DO_BRANCH(regs.flags.c == 0, "BCC");

        case 0xa: {
            regs.y = getPcByte();
            regs.setNZ(regs.y);
            return INSN_DONE(2, "LDY #0x%02x", regs.y);
        }

        case 0xb: DO_BRANCH(regs.flags.c == 1, "BCS");

        case 0xc: {
            Byte imm = getPcByte();
            doAddSub(regs.y, imm, false, true);
            return INSN_DONE(2, "CPY #0x%02x", imm);
        }

        case 0xd: DO_BRANCH(regs.flags.z == 0, "BNE");

        case 0xe: {
            Byte imm = getPcByte();
            doAddSub(regs.x, imm, false, true);
            return INSN_DONE(2, "CPX #0x%02x", imm);
        }

        case 0xf: DO_BRANCH(regs.flags.z == 1, "BEQ");
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
            Byte b = bus->memRead8(addr);
            regs.flags.z = (regs.a & b) == 0;
            regs.flags.v = !!(b & bit(6));
            regs.flags.n = !!(b & bit(7));
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
        case 0x0:
            regs.flags.c = !!(regs.a & 0x80);
            regs.a <<= 1;
            regs.setNZ(regs.a);
            return INSN_DONE(2, "ASL");

        case 0x2: {
            bool origCarry = regs.flags.c;
            regs.flags.c = !!(regs.a & 0x80);
            regs.a = (regs.a << 1) | (origCarry ? 0x01 : 0x0);
            regs.setNZ(regs.a);
            return INSN_DONE(2, "ROL");
        }

        case 0x4:
            regs.flags.c = !!(regs.a & 0x01);
            regs.a >>= 1;
            regs.setNZ(regs.a);
            return INSN_DONE(2, "LSR");

        case 0x6: {
            bool origCarry = regs.flags.c;
            regs.flags.c = !!(regs.a & 0x01);
            regs.a = (regs.a >> 1) | (origCarry ? 0x80 : 0x0);
            regs.setNZ(regs.a);
            return INSN_DONE(2, "ROR");
        }

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
    bool hasIndexReg = !!(opcode & bit(4));

    Word addr = isTwoByte ? getPcWord() : getPcByte();
    Word mask = isTwoByte ? 0xffff : 0xff;
    int fieldWidth = isTwoByte ? 4 : 2;

    // Handle the special cases here: index reg is Y, and not RMW instructions
    Word specialEffAddr = hasIndexReg ? addr + regs.y : addr;
    switch ((opcode >> 4) & ~1) {
        case 0x8:
            bus->memWrite8(specialEffAddr, regs.x);
            return INSN_DONE(3 + hasIndexReg, "STX $0x%0*x%s", fieldWidth, addr, hasIndexReg ? ", Y" : "");

        case 0xa:
            regs.x = bus->memRead8(specialEffAddr);
            regs.setNZ(regs.x);
            // XXX: instruction 0xBE should have page crossing cost?
            return INSN_DONE(3 + hasIndexReg, "LDX $0x%0*x%s", fieldWidth, addr, hasIndexReg ? ", Y" : "");
    }

    Word effAddr = hasIndexReg ? addr + regs.x : addr;
    const char* str;

    Byte b = bus->memRead8(effAddr & mask);
    switch ((opcode >> 4) & ~1) {
        case 0x0:
            regs.flags.c = !!(b & 0x80);
            b <<= 1;
            regs.setNZ(b);
            str = "ASL";
            break;

        case 0x2: {
            bool origCarry = regs.flags.c;
            regs.flags.c = !!(b & 0x80);
            b = (b << 1) | (origCarry ? 0x01 : 0x0);
            regs.setNZ(b);
            str = "ROL";
            break;
        }

        case 0x4:
            regs.flags.c = !!(b & 0x01);
            b >>= 1;
            regs.setNZ(b);
            str = "LSR";
            break;

        case 0x6: {
            bool origCarry = regs.flags.c;
            regs.flags.c = !!(b & 0x01);
            b = (regs.a >> 1) | (origCarry ? 0x80 : 0x0);
            regs.setNZ(b);
            str = "ROR";
            break;
        }

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
    // XXX verify cycle count
    return INSN_DONE(6 + hasIndexReg, "%s $0x%0*x%s", str, fieldWidth, addr, hasIndexReg ? ", X" : "");
}

long Cpu::handleColumn159D(Byte opcode) {
    INSN_DBG_DECL();
    // insn bytes: 01, 05, 09, 0D, 11, 15, 19, 1D
    // addr modes: izx, zp, imm, abs, izy, zpx, aby, abx
    int amode = (opcode >> 2) & 0x7;
    int action = (opcode >> 4) & ~1;

    bool isImm = amode == 2;
    bool isWrite = action == 8;

    const char* fmt;
    Byte b;
    Word addrVal;
    Word effectiveAddr;
    switch (amode) {
        case 0:
            fmt = "%s ($0x%02x, X)";
            addrVal = getPcByte();
            effectiveAddr = bus->memRead16((addrVal + regs.x) & 0xff);
            break;

        case 1:
            fmt = "%s $0x%02x";
            addrVal = getPcByte();
            effectiveAddr = addrVal;
            break;

        case 2:
            fmt = "%s #0x%02x";
            addrVal = b = getPcByte();
            effectiveAddr = 0; // Quiet compiler warning
            break;

        case 3:
            fmt = "%s 0x%04x";
            addrVal = getPcWord();
            effectiveAddr = addrVal;
            break;

        case 4:
            fmt = "%s (0x%02x), Y";
            addrVal = getPcByte();
            effectiveAddr = bus->memRead16(addrVal) + regs.y;
            break;

        case 5:
            fmt = "%s 0x%02x, X";
            addrVal = getPcByte();
            effectiveAddr = (addrVal + regs.x) & 0xff;
            break;

        case 6:
            fmt = "%s 0x%04x, Y";
            addrVal = getPcWord();
            effectiveAddr = addrVal + regs.y;
            break;

        case 7:
            fmt = "%s 0x%04x, X";
            addrVal = getPcWord();
            effectiveAddr = addrVal + regs.x;
            break;

        default:
            unreachable();
    }

    if (!isImm && !isWrite)
        b = bus->memRead8(effectiveAddr);

    const char* str;
    switch (action) {
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
            bus->memWrite8(effectiveAddr, regs.a);
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

        default:
            unreachable();
    }

    // XXX cycle count
    return INSN_DONE(4, fmt, str, addrVal);
}
