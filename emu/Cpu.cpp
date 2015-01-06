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


void Cpu::reset() {
    regs = Regs();
}

long Cpu::tick() {
    return 4;
}
