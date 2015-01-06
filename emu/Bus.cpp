#include "Bus.hpp"
#include "BusUtil.hpp"
#include "Gpu.hpp"
#include "Joypad.hpp"
#include "Rom.hpp"
#include "Sound.hpp"

#include <algorithm>

void Bus::memAccess(Word address, Byte* pData, bool isWrite, MemAccessType accessType) {
    if (address <= 0xff) {
        rom->cartRomAccess(address, pData, isWrite);
    } else if (address <= 0x7fff) {
        rom->cartRomAccess(address, pData, isWrite);
    } else if (address <= 0xbfff) {
        rom->cartRamAccess(address & 0x1fff, pData, isWrite);
    } else if (address <= 0xfdff) {
        BusUtil::arrayMemAccess(ram, address & 0x1fff, pData, isWrite);
    } else if (address == 0xff00) {
        joypad->regAccess(pData, isWrite);
    } else if (address == 0xff0f) {
        BusUtil::simpleRegAccess(&irqsPending, pData, isWrite, 0x1f);
    } else if (address >= 0xff10 && address <= 0xff3f) {
        sound->registerAccess(address, pData, isWrite);
    } else if (address >= 0xff80 && address <= 0xfffe) {
        BusUtil::arrayMemAccess(hram, address - 0xff80, pData, isWrite);
    } else if (address == 0xffff) {
        BusUtil::simpleRegAccess(&irqsEnabled, pData, isWrite, 0x1f);
    } else {
        if (isWrite) {
            log->warn("Unhandled write (0x%02x) to address 0x%04X", *pData, address);
        } else {
            log->warn("Unhandled read from address 0x%04X", address);
        }
    }

#ifndef CONFIG_NO_INSN_TRACE
    if (accessType) {
        log->logMemoryAccess(address, *pData, isWrite, accessType);
    }
#endif
}

Byte Bus::memRead8(Word address, MemAccessType accessType) {
    Byte value = 0;
    memAccess(address, &value, false, accessType);
    return value;
}

void Bus::memWrite8(Word address, Byte value, MemAccessType accessType) {
    memAccess(address, &value, true, accessType);
}

Word Bus::memRead16(Word address, MemAccessType accessType) {
    return memRead8(address, accessType) | (memRead8(address + 1, accessType) << 8);
}

void Bus::memWrite16(Word address, Word value, MemAccessType accessType) {
    memWrite8(address, (Byte)(value), accessType);
    memWrite8(address + 1, (Byte)(value >> 8), accessType);
}

void Bus::raiseIrq(IrqSet irqs) {
    irqsPending |= irqs; // TODO: should this be masked with irqsEnabled???
}

void Bus::ackIrq(Irq irq) {
    if (!(irqsPending & (1 << irq))) {
        log->warn("IRQ %d not pending?", irq);
    }
    irqsPending &= ~(1 << irq);
}

IrqSet Bus::getEnabledIrqs() {
    return irqsEnabled;
}

IrqSet Bus::getPendingIrqs() {
    return irqsPending & irqsEnabled;
}

