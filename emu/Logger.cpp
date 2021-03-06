#include "Bus.hpp"
#include "Cpu.hpp"
#include "Gpu.hpp"

#include <stdio.h>
#include <stdarg.h>

void Logger::logInsn(Bus* bus, Regs* regs, int cycles, Word newPC, const char* fmt, ...) {
    if (!(logFlags & Log_Insns)) {
        return;
    }

    char hexdumpBuf[sizeof("01 02 03")];
    unsigned insnCount = (Word)(newPC - regs->pc);
    assert(insnCount <= 3);
    for (unsigned i = 0; i < insnCount; i++) {
        sprintf(&hexdumpBuf[i * 3], "%02X ", bus->memRead8(regs->pc + i, NULL));
    }
    hexdumpBuf[3 * insnCount - 1] = 0;

    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    logImpl("[insn %05ld/%03d/%08ld] 0x%04X: %8s => %-32s "
                    "A: 0x%02x | X: 0x%02x | Y: 0x%02x | SP: 0x%02x | Flags: %c%c%c%c%c | Cycles: %d",
            currentFrame, currentScanline, currentCycle % ScanlineCycles,
            regs->pc, hexdumpBuf, buf, regs->a, regs->x, regs->y, regs->sp,
            regs->flags.n ? 'N' : '-', regs->flags.v ? 'V' : '-',
            regs->flags.z ? 'Z' : '-', regs->flags.c ? 'C' : '-',
            regs->flags.i ? '.' : '!',
            cycles);
}

void Logger::logMemoryAccess(Word addr, Byte data, bool isWrite, MemAccessType accessType) {
    if (!(logFlags & Log_MemoryAccesses)) {
        return;
    }
    logImpl("[mem %s (%s)] 0x%04x: %02x", isWrite ? "wr" : "rd", accessType, addr, data);
}

void Logger::warn(const char* fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);

    vsnprintf(buf, sizeof(buf), fmt, ap);

    va_end(ap);
    logImpl("[warn] %s", buf);
}

void Logger::logDebug(const char* fmt, ...) {
    if (!logFlags) {
        return;
    }

    char buf[256];
    va_list ap;
    va_start(ap, fmt);

    vsnprintf(buf, sizeof(buf), fmt, ap);

    va_end(ap);
    logImpl("%s", buf);
}
