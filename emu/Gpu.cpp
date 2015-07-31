#include "Bus.hpp"
#include "Gpu.hpp"
#include "Rom.hpp"

bool Gpu::tick(long cycles) {
    cycleResidue += cycles;
    if (cycleResidue >= ScanlineCycles) {
        cycleResidue -= ScanlineCycles;

        if (scanline < VblankStartScanline) {
            renderScanline();
        }

        scanline++;
        if (scanline == VblankStartScanline) {
            regs.inVblank = 1;
        } else if (scanline >= ScanlinesPerFrame) {
            scanline = 0;
            frame++;
            regs.inVblank = 0;
        }
    }
    return regs.inVblank && regs.vblankIrqEnabled;
}

Byte Gpu::drawTilePixel(Byte* tile, unsigned x, unsigned y, bool large) {
    //Byte height = large ? 8 : 8; // WIP

    Byte lsbs = tile[y + 0];
    Byte msbs = tile[y + 8];

    return (!!(lsbs & (0x80 >> x))) |
           ((!!(msbs & (0x80 >> x))) << 1);
}

void Gpu::renderScanline() {
    for (unsigned i = 0; i < ScreenWidth; i++) {
        if (!regs.spritesEnabled && !regs.bgEnabled) {
            framebuffer[scanline][i] = 0;
            continue;
        }

        // WIP
        unsigned scrollY = 0;
        unsigned scrollX = 0;

        // XXX: Really direct pointer access?
        Byte* bgPatternBase = &bus->getRom()->getChrRom()[regs.bgPatternTableAddr ? 0x1000 : 0];
        unsigned bgY = scanline + scrollY;
        unsigned bgTileY = (bgY / 8) % 32;
        unsigned bgTileYBit = bgY % 8;

        Byte* bgTileBase = vram;

        Byte bgColor = 0;
        Byte pixel = 0;
        if (regs.bgEnabled) {
            unsigned bgX = i + scrollX;
            unsigned bgTileX = (bgX / 8) % 32;
            unsigned bgTileXBit = bgX % 8;

            Byte tileNum = bgTileBase[bgTileY * 32 + bgTileX];
            bgColor = drawTilePixel(bgPatternBase + 16 * tileNum, bgTileXBit, bgTileYBit, false);
            //pixel = applyPalette(regs.bgp, bgColor);
            pixel = bgColor;
        }
        framebuffer[scanline][i] = pixel;
    }
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
            BusUtil::simpleRegAccess(&regs.status, pData, isWrite, 0);
            if (!isWrite) {
                regs.inVblank = 0;
                regs.vramAddrRegSelect = 0;
            }
            break;

        case 0x2006:
            if (!regs.vramAddrRegSelect) {
                BusUtil::simpleRegAccess(&regs.vramAddrHi, pData, isWrite, (1 << 6) - 1);
            } else {
                BusUtil::simpleRegAccess(&regs.vramAddrLo, pData, isWrite);
            }
            regs.vramAddrRegSelect = !regs.vramAddrRegSelect;
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

            regs.vramAddr += regs.vramPortAddressIncr ? 32 : 1;
            regs.vramAddrHi &= (1 << 6) - 1;
            break;
        }

        default:
            log->warn("Unhandled GPU register %s to register %04X", isWrite ? "write" : "read", reg);
            break;
    }
}
