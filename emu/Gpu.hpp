#pragma once

#include "BusUtil.hpp"
#include "Irq.hpp"
#include "Logger.hpp"
#include "Platform.hpp"
#include "Serializer.hpp"

#include <cstring>

/*
 * Gpu timings (NTSC, 256x224):
 *  - Master Clock (X1)  21.47727  MHz
 *  - CPU Clock          1.7897725 MHz (X1/12)
 *  - Dot Clock          5.3693175 MHz (X1/4)
 *
 *  CPU Clks per Scanline: 113.6666 => 1364   clks/scanline
 *                                  => 357368 clks/frame
 */

enum {
    ScreenWidth = 256,
    ScreenHeight = 240,

    VblankStartScanline = 240,
    ScanlinesPerFrame = 262,

    ScanlineCycles = 1364,
};

struct GpuRegs {
    union {
        Byte ctrl1;
        struct {
            Byte nameTableScrollAddr : 2;
            Byte vramPortAddressIncr :1;
            Byte spritePatternTableAddr : 1;
            Byte bgPatternTableAddr : 1;
            Byte largeSprites : 1;
            Byte masterSlaveSelect : 1;
            Byte vblankIrqEnabled : 1;
        };
    };
    union {
        Byte ctrl2;
        struct {
            Byte monochrome : 1;
            Byte bgClipping : 1;
            Byte spriteClipping : 1;
            Byte bgEnabled : 1;
            Byte spritesEnabled : 1;
            Byte colorEmphasis : 3;
        };
    };
    union {
        Byte status;
        struct {
            Byte unused : 5;
            Byte lostSprites : 1;
            Byte spriteZeroHit : 1;
            Byte inVblank : 1;
        };
    };
    Byte spriteRamAddr;

    Byte scrollOffsetHi, scrollOffsetLo;
    union {
        Word vramAddr;
        struct {
            Byte vramAddrLo, vramAddrHi;
        };
    };
    Byte vramReadLatch;
    bool vramAddrRegSelect;
};

struct alignas(1) OamFlags {
    Byte palette : 2;
    Byte unused : 3;
    Byte lowPriority : 1;
    Byte horizFlip : 1;
    Byte vertFlip : 1;
};
static_assert(sizeof(OamFlags) == 1, "sizeof OamFlags");

struct alignas(1) Sprite {
    Byte y;
    Byte tileNumber;
    OamFlags flags;
    Byte x;
};
static_assert(sizeof(Sprite) == 4, "sizeof Sprite");

class Gpu {
    Logger* log;
    Bus* bus;

    bool renderEnabled;
    long frame;
    long scanline;
    int cycleResidue;

    Byte framebuffer[ScreenHeight][ScreenWidth];
    Byte vram[4096]; // Really 2k, but pretend that potential cart RAM for name+attribute tables is here as well
    Byte paletteRam[0x20]; // keep this after vram!!!
    union {
        Byte spriteRam[256];
        Sprite sprites[64];
    };

    Byte visibleSprites[8];
    GpuRegs regs;

    void sortSprites();
    void renderScanline();
    static Byte drawTilePixel(Byte* tile, unsigned int x, unsigned int y, bool large, Sprite* sprite);

public:
    static const Byte colorTable[64 * 3];

    Gpu(Logger* log, Bus* bus) :
            log(log),
            bus(bus),
            renderEnabled(true),
            frame(0),
            scanline(0),
            cycleResidue(0) {
        std::memset(&framebuffer[0][0], 0, sizeof(framebuffer));
        std::memset(&vram[0], 0, sizeof(vram));
        std::memset(&paletteRam[0], 0, sizeof(paletteRam));
        std::memset(&spriteRam[0], 0, sizeof(spriteRam));
        std::memset(&regs, 0, sizeof(regs));
    }

    long getCurrentScanline() { return scanline; }
    long getCurrentFrame() { return frame; }
    Byte* getFramebuffer() { return &framebuffer[0][0]; }
    Byte* getVram() { return &vram[0]; }
    GpuRegs* getRegs() { return &regs; }
    void setRenderEnabled(bool renderEnabled) { this->renderEnabled = renderEnabled; }

    void registerAccess(Word reg, Byte* pData, bool isWrite);
    void spriteDmaWrite(Byte addr, Byte data);
    bool tick(long cycles);
    void serialize(Serializer& ser);
};
