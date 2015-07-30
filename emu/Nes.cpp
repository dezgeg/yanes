#include "Nes.hpp"

void Nes::runOneInstruction() {
    long newFrame = gpu.getCurrentFrame();
    log->setTimestamp(newFrame, gpu.getCurrentScanline(), currentCycle);

    if (joypad.tick()) {
        bus.raiseIrq(bit(Irq_Joypad));
    }

    int cycleDelta = 12 * cpu.tick();

    IrqSet gpuIrqs = gpu.tick(cycleDelta);
    bus.raiseIrq(gpuIrqs);
    sound.tick(cycleDelta);
    currentCycle += cycleDelta;
}
