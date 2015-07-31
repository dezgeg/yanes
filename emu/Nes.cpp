#include "Nes.hpp"

void Nes::runOneInstruction() {
    long newFrame = gpu.getCurrentFrame();
    log->setTimestamp(newFrame, gpu.getCurrentScanline(), currentCycle);

    joypad.tick();

    int cycleDelta = 12 * cpu.tick();

    bool gpuIrq = gpu.tick(cycleDelta);
    bus.setNmiState(gpuIrq);
    sound.tick(cycleDelta);
    currentCycle += cycleDelta;
}
