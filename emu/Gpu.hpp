#pragma once

#include "BusUtil.hpp"
#include "Irq.hpp"
#include "Logger.hpp"
#include "Platform.hpp"

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
            Byte vblankIrq : 1;
        };
    };
    union {
        Byte ctrl2;
        struct {
            Byte monochrome : 1;
            Byte bgClipping : 1;
            Byte spriteClipping : 1;
            Byte bgVisibility : 1;
            Byte spriteVisiblity : 1;
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
    Byte scrollOffsetHi, scrollOffsetLo;
    union {
        Word vramAddr;
        struct {
            Byte vramAddrLo, vramAddrHi;
        };
    };
    bool vramAddrRegSelect;
};

class Gpu {
    Logger* log;

    bool renderEnabled;
    long frame;
    long scanline;
    int cycleResidue;

    Byte framebuffer[ScreenHeight][ScreenWidth];
    Byte vram[0x4000];
    GpuRegs regs;

    void renderScanline();

public:
    Gpu(Logger* log) :
            log(log),
            renderEnabled(true),
            frame(0),
            scanline(0),
            cycleResidue(0) {
        std::memset(&framebuffer[0][0], 0, sizeof(framebuffer));
        std::memset(&vram[0], 0, sizeof(vram));
        std::memset(&regs, 0, sizeof(regs));
    }

    long getCurrentScanline() { return scanline; }
    long getCurrentFrame() { return frame; }
    Byte* getFramebuffer() { return &framebuffer[0][0]; }
    Byte* getVram() { return &vram[0]; }
    GpuRegs* getRegs() { return &regs; }
    void setRenderEnabled(bool renderEnabled) { this->renderEnabled = renderEnabled; }

    void registerAccess(Word reg, Byte* pData, bool isWrite);
    IrqSet tick(long cycles);
};
