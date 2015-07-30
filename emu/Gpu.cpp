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
            regs.inVblank = 0;
            break;

        case 0x2006:
            if (!regs.vramAddrRegSelect) {
                BusUtil::simpleRegAccess(&regs.vramAddrHi, pData, isWrite, (1 << 6) - 1);
            } else {
                BusUtil::simpleRegAccess(&regs.vramAddrLo, pData, isWrite);
            }
            break;

        case 0x2007: {
            // XXX: latch behaviour of VRAM reads
            if (regs.vramAddr < 0x2000) {
                log->warn("VRAM %s to pattern table addr %04X ???", isWrite ? "write" : "read", regs.vramAddr);
                // forward to bus maybe?
            } else if (regs.vramAddr >= 0x3f00 && regs.vramAddr < 0x3f20) {
                BusUtil::arrayMemAccess(paletteRam, regs.vramAddr - 0x3f00, pData, isWrite);
            } else {
                Word address = (regs.vramAddr - 0x2000) % sizeof(vram);
                BusUtil::arrayMemAccess(vram, address, pData, isWrite);
            }
            log->warn("VRAM %s [%02x] to addr %04X", isWrite ? "write" : "read", *pData, regs.vramAddr);

            regs.vramAddr += regs.vramPortAddressIncr ? 16 : 1;
            regs.vramAddrHi &= (1 << 6) - 1;
            break;
        }

        default:
            log->warn("Unhandled GPU register %s to register %04X", isWrite ? "write" : "read", reg);
            break;
    }
}
