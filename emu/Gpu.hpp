#pragma once

#include "BusUtil.hpp"
#include "Irq.hpp"
#include "Logger.hpp"
#include "Platform.hpp"

#include <cstring>
#include <zconf.h>

enum {
    ScreenWidth = 256,
    ScreenHeight = 240,

    ScanlineCycles = 456,
};

struct GpuRegs {
};

class Gpu {
    Logger* log;

    bool renderEnabled;
    long frame;
    int cycleResidue;
    Byte framebuffer[ScreenHeight][ScreenWidth];

    GpuRegs regs;
    void renderScanline();

public:
    Gpu(Logger* log) :
            log(log),
            renderEnabled(true),
            frame(0),
            cycleResidue(0) {
        std::memset(&framebuffer[0][0], 0, sizeof(framebuffer));
        std::memset(&regs, 0, sizeof(regs));
    }

    int getCurrentScanline() { return 0; }
    long getCurrentFrame() {
        // HACK
        return frame++;
    }
    Byte* getFramebuffer() { return &framebuffer[0][0]; }
    Byte* getVram() { return nullptr; }
    GpuRegs* getRegs() { return &regs; }
    void setRenderEnabled(bool renderEnabled) { this->renderEnabled = renderEnabled; }

    void registerAccess(Word reg, Byte* pData, bool isWrite);
    IrqSet tick(long cycles);
};
