#include "Gpu.hpp"


IrqSet Gpu::tick(long cycles) {
    cycleResidue += cycles;
    if (cycleResidue >= ScanlineCycles) {
        cycleResidue -= ScanlineCycles;
        scanline++;

        if (scanline == VblankStartScanline) {
            regs.inVblank = 1;
        }

        if (scanline >= ScanlinesPerFrame) {
            scanline = 0;
            frame++;
        }
    }
    return 0;
}

void Gpu::renderScanline() {
}

void Gpu::registerAccess(Word reg, Byte* pData, bool isWrite) {
    switch (reg) {
        case 0x2000:
        BusUtil::simpleRegAccess(&regs.ctrl1, pData, isWrite);
            break;
        case 0x2001:
            BusUtil::simpleRegAccess(&regs.ctrl2, pData, isWrite);
            break;
        case 0x2002:
            BusUtil::simpleRegAccess(&regs.status, pData, isWrite);
            break;
        default:
            log->warn("Unhandled GPU register %s to register %04X", isWrite ? "write" : "read", reg);
            break;
    }
}
