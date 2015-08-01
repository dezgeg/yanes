#include "Nes.hpp"

void Nes::runOneInstruction() {
    long newFrame = gpu.getCurrentFrame();
    log->setTimestamp(newFrame, gpu.getCurrentScanline(), currentCycle);

    int cycleDelta;
    if (bus.tickDma()) {
        cycleDelta = 8;
    } else {
        cycleDelta = 12 * cpu.tick();
    }

    joypad.tick();

    bool gpuIrq = gpu.tick(cycleDelta);
    bus.setNmiState(gpuIrq);
    sound.tick(cycleDelta);
    currentCycle += cycleDelta;
}
