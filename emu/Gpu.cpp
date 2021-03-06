#include "Bus.hpp"
#include "Gpu.hpp"
#include "Rom.hpp"
#include "Serializer.hpp"

#include <algorithm>

const Byte Gpu::colorTable[] = {
        3, 3, 3, 4, 1, 0, 6, 0, 0, 6, 2, 3, 3, 0, 4, 3, 0, 5, 0, 1, 5, 0, 2, 4, 0, 2, 3, 0, 2, 1, 1, 3, 0, 0, 4, 0, 2,
        2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 5, 5, 6, 3, 0, 7, 2, 0, 7, 0, 4, 7, 0, 5, 4, 0, 7, 0, 0, 7, 0, 3, 6, 0, 3,
        4, 0, 4, 1, 0, 4, 0, 3, 5, 0, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 7, 7, 7, 5, 3, 7, 4, 4, 7, 3, 6, 7, 0, 7,
        7, 3, 7, 0, 4, 7, 0, 5, 7, 0, 6, 6, 0, 6, 3, 0, 7, 0, 6, 7, 2, 7, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 7, 7, 7,
        6, 5, 7, 5, 6, 7, 5, 7, 7, 4, 7, 5, 5, 7, 4, 6, 7, 2, 7, 7, 3, 7, 7, 2, 7, 5, 3, 7, 4, 6, 7, 2, 7, 6, 4, 0, 0,
        0, 0, 0, 0, 0, 0, 0
};

static unsigned normalizePaletteIndex(unsigned i) {
    // XXX: does this work for sprites?
    if ((i & 0x3) == 0x0) {
        return i & 0x10 ? i & ~0x10 : 0;
    }
    return i;
}

void Gpu::sortSprites() {
    unsigned numVisibleSprites = 0;
    for (Byte i = 0; i < 64; i++) {
        int spriteY = sprites[i].y + 1;
        if (scanline < spriteY || scanline >= spriteY + 8) {
            continue;
        }
        if (numVisibleSprites >= 8) {
            regs.lostSprites = 1;
        } else {
            visibleSprites[numVisibleSprites++] = i;
        }
    }

    std::sort(visibleSprites, visibleSprites + numVisibleSprites, [ this ](Byte a, Byte b) {
        return sprites[a].x < sprites[b].x;
    });

    for (unsigned i = numVisibleSprites; i < 8; i++) {
        visibleSprites[i] = 0xff;
    }
}

bool Gpu::tick(long cycles) {
    cycleResidue += cycles;
    if (cycleResidue >= ScanlineCycles) {
        cycleResidue -= ScanlineCycles;

        if (scanline < VblankStartScanline) {
            sortSprites();
            renderScanline();
        }

        scanline++;
        if (scanline == VblankStartScanline) {
            regs.inVblank = 1;
        } else if (scanline >= ScanlinesPerFrame) {
            scanline = 0;
            frame++;
            regs.inVblank = 0;
            regs.spriteZeroHit = 0;
        }
    }
    return regs.inVblank && regs.vblankIrqEnabled;
}

Byte Gpu::drawTilePixel(Byte* tile, unsigned x, unsigned y, bool large, Sprite* sprite) {
    //Byte height = large ? 8 : 8; // WIP
    if (sprite && sprite->flags.vertFlip) {
        y = 7 - y;
    }

    Byte lsbs = tile[y + 0];
    Byte msbs = tile[y + 8];

    if (sprite && sprite->flags.horizFlip) {
        lsbs = reverseBits(lsbs);
        msbs = reverseBits(msbs);
    }

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
        Byte bgPaletteIndex = 0;
        if (regs.bgEnabled) {
            unsigned bgX = i + scrollX;
            unsigned bgTileX = (bgX / 8) % 32;
            unsigned bgTileXBit = bgX % 8;

            Byte tileNum = bgTileBase[bgTileY * 32 + bgTileX];
            bgColor = drawTilePixel(bgPatternBase + 16 * tileNum, bgTileXBit, bgTileYBit, false, nullptr);

            unsigned attrTableY = bgTileY / 4;
            unsigned attrTableX = bgTileX / 4;
            Byte attrTableVal = bgTileBase[0x3c0 + 8 * attrTableY + attrTableX];

            unsigned attrTableBitY = (bgTileY & 0x2) >> 1;
            unsigned attrTableBitX = (bgTileX & 0x2) >> 1;

//            log->warn("For tile (%02d, %02d): attr: (%02d, %02d), bits: (%02d, %02d)",
//                      bgTileX, bgTileY, attrTableX, attrTableY, attrTableBitX, attrTableBitY);

            unsigned shift = 4 * attrTableBitY + 2 * attrTableBitX;

            unsigned paletteTop = (attrTableVal >> shift) & 0x3;
            bgPaletteIndex = (paletteTop << 2) | bgColor;
        }

        Byte spriteColor = 0;
        Byte spritePaletteIndex = 0;
        bool spriteHiPriority = false;
        bool isSpriteZero = false;

        if (regs.spritesEnabled) {
            // XXX: Really direct pointer access?
            Byte* spritePatternBase = &bus->getRom()->getChrRom()[regs.spritePatternTableAddr ? 0x1000 : 0];

            for (unsigned j = 0; j < 8; j++) {
                unsigned spriteIndex = visibleSprites[j];
                if (spriteIndex == 0xff) {
                    continue;
                }

                Sprite* spr = &sprites[spriteIndex];
                if (i < spr->x || i >= unsigned(spr->x) + 8) {
                    continue;
                }

                spriteColor = drawTilePixel(spritePatternBase + 16 * spr->tileNumber, i - spr->x, scanline - spr->y - 1,
                                            false, spr);
                if (!spriteColor) {
                    continue;
                }

                spritePaletteIndex = 0x10 + 0x4 * spr->flags.palette + spriteColor;
                spriteHiPriority = !spr->flags.lowPriority;
                isSpriteZero = spriteIndex == 0;
                break;
            }
        }

        Byte pixel;
        if (!spriteHiPriority || spriteColor == 0) {
            pixel = paletteRam[normalizePaletteIndex(bgPaletteIndex)];
        } else {
            pixel = paletteRam[normalizePaletteIndex(spritePaletteIndex)];
        }

        if (spriteColor && bgColor && isSpriteZero) {
            regs.spriteZeroHit = 1;
        }

        framebuffer[scanline][i] = pixel;
    }
}

void Gpu::registerAccess(Word reg, Byte* pData, bool isWrite) {
    reg &= 0x7;
    switch (reg) {
        case 0:
            BusUtil::simpleRegAccess(&regs.ctrl1, pData, isWrite);
            break;
        case 1:
            BusUtil::simpleRegAccess(&regs.ctrl2, pData, isWrite);
            break;
        case 2:
            BusUtil::simpleRegAccess(&regs.status, pData, isWrite, 0);
            if (!isWrite) {
                regs.inVblank = 0;
                regs.vramAddrRegSelect = 0;
            }
            break;

        case 3:
            BusUtil::simpleRegAccess(&regs.spriteRamAddr, pData, isWrite);
            break;

        case 4:
            if (isWrite) {
                spriteDmaWrite(regs.spriteRamAddr++, *pData);
            } else {
                *pData = spriteRam[regs.spriteRamAddr];
            }
            break;

        case 6:
            if (!regs.vramAddrRegSelect) {
                BusUtil::simpleRegAccess(&regs.vramAddrHi, pData, isWrite, (1 << 6) - 1);
            } else {
                BusUtil::simpleRegAccess(&regs.vramAddrLo, pData, isWrite);
            }
            regs.vramAddrRegSelect = !regs.vramAddrRegSelect;
            break;

        case 7: {
            if (regs.vramAddr < 0x2000) {
                if (isWrite) {
                    log->warn("VRAM %s to pattern table addr %04X ???", isWrite ? "write" : "read", regs.vramAddr);
                } else {
                    *pData = bus->getRom()->getChrRom()[regs.vramAddr];
                }
            } else if (regs.vramAddr >= 0x3f00 && regs.vramAddr < 0x3f20) {
                unsigned paletteAddr = regs.vramAddr - 0x3f00;
                if (paletteAddr & 0x10 && (paletteAddr & 0x3) == 0x0) {
                    paletteAddr &= ~0x10u;
                }
                if (isWrite) {
                    *pData &= 0x3f;
                } else {
                    regs.vramReadLatch = *pData;
                }
                BusUtil::arrayMemAccess(paletteRam, paletteAddr, pData, isWrite);
            } else {
                Word address = regs.vramAddr - 0x2000; // FIXME: mirror properly based on ROM mapper
                BusUtil::arrayMemAccess(vram, address, pData, isWrite);
            }
//            log->warn("VRAM %s [%02x] to addr %04X", isWrite ? "write" : "read", *pData, regs.vramAddr);

            if (!isWrite) {
                std::swap(regs.vramReadLatch, *pData);
            }

            regs.vramAddr += regs.vramPortAddressIncr ? 32 : 1;
            regs.vramAddrHi &= (1 << 6) - 1;
            break;
        }
    }
}

void Gpu::spriteDmaWrite(Byte addr, Byte data) {
    spriteRam[addr] = data;
}

void Gpu::serialize(Serializer& ser) {
    ser.handleObject("Gpu.frame", frame);
    ser.handleObject("Gpu.scanline", scanline);
    ser.handleObject("Gpu.cycleResidue", cycleResidue);
    ser.handleObject("Gpu.framebuffer", framebuffer);
    ser.handleObject("Gpu.vram", vram);
    ser.handleObject("Gpu.paletteRam", paletteRam);
    ser.handleObject("Gpu.sprites", sprites);
    ser.handleObject("Gpu.visibleSprites", visibleSprites);
    ser.handleObject("Gpu.regs", regs);
}
