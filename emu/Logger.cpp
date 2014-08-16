#include "Bus.hpp"
#include "Cpu.hpp"
#include "Logger.hpp"

#include <stdio.h>
#include <stdarg.h>

void Logger::logInsn(Bus* bus, Regs* regs, int cycles, Word newPC, const char* fmt, ...)
{
    if (!insnLoggingEnabled || regs->pc <= 0xff)
        return;

    char hexdumpBuf[sizeof("01 02 03")];
    unsigned insnCount = newPC - regs->pc;
    for (unsigned i = 0; i < insnCount; i++)
        sprintf(&hexdumpBuf[i * 3], "%02X ", bus->memRead8(regs->pc + i));
    hexdumpBuf[3 * insnCount - 1] = 0;

    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    printf("[insn %05ld/%03d/%08ld] 0x%04X: %8s => %-32s "
           "A: 0x%02x | BC: 0x%04x | DE: 0x%04x | HL: 0x%04x | SP: 0x%04x | Flags: %c%c%c%c | Cycles: %d\n",
           currentFrame, currentScanline, currentCycle,
           regs->pc, hexdumpBuf, buf, regs->a, regs->bc, regs->de, regs->hl, regs->sp,
           regs->flags.z ? 'Z' : '-', regs->flags.n ? 'N' : '-',
           regs->flags.h ? 'H' : '-', regs->flags.c ? 'C' : '-',
           cycles);
}

void Logger::warn(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    printf("[warn] ");
    vprintf(fmt, ap);
    putchar('\n');

    va_end(ap);
}
