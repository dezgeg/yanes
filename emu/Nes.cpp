#include "Nes.hpp"

void Nes::runOneInstruction() {
    long newFrame = gpu.getCurrentFrame();
    log->setTimestamp(newFrame, gpu.getCurrentScanline(), currentCycle);

    if (joypad.tick()) {
        bus.raiseIrq(bit(Irq_Joypad));
    }

    int cycleDelta = cpu.tick();

    if (serial.tick(cycleDelta)) {
        bus.raiseIrq(bit(Irq_Serial));
    }

    IrqSet gpuIrqs = gpu.tick(cycleDelta);
    bus.raiseIrq(gpuIrqs);
    sound.tick(cycleDelta);
    currentCycle += cycleDelta;
}
