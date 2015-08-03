#include "Bus.hpp"
#include "BusUtil.hpp"
#include "Gpu.hpp"
#include "Joypad.hpp"
#include "Rom.hpp"
#include "Sound.hpp"

void Bus::memAccess(Word address, Byte* pData, bool isWrite, MemAccessType accessType) {
    if (address <= 0x1fff) {
        BusUtil::arrayMemAccess(ram, address & 0x7ff, pData, isWrite);
    } else if (address <= 0x3fff) {
        gpu->registerAccess(address, pData, isWrite);
    } else if (address == 0x4014) {
        if (!isWrite) {
            log->warn("Read from sprite RAM DMA???");
            return;
        }
        spriteDmaBytes = 256;
        spriteDmaBank = *pData;
    } else if (address >= 0x4016 && address <= 0x4017) {
        joypad->regAccess(address, pData, isWrite);
    } else if (address >= 0x8000) {
        rom->cartRomAccess(address - 0x8000, pData, isWrite);
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

void Bus::setNmiState(bool state) {
    if (lastNmiState != state && state)
        irqsPending |= Irq_NMI;
    else
        irqsPending &= ~Irq_NMI;
    //log->warn("ASDASD state: %d last: %d, pend: %d", (int)state, (int)lastNmiState, (int)irqsPending);
    lastNmiState = state;
}

IrqSet Bus::getPendingIrqs() {
    return irqsPending;
}

bool Bus::tickDma() {
    if (!spriteDmaBytes)
        return false;
    spriteDmaBytes--;

    gpu->spriteDmaWrite(spriteDmaBytes, memRead8((spriteDmaBank << 8) | spriteDmaBytes, "Sprite DMA"));
    return true;
}

void Bus::serialize(Serializer& ser) {
    ser.handleObject("Bus.irqsPending", irqsPending);
    ser.handleObject("Bus.lastNmiState", lastNmiState);
    ser.handleObject("Bus.spriteDmaBytes", spriteDmaBytes);
    ser.handleObject("Bus.spriteDmaBank", spriteDmaBank);
    ser.handleObject("Bus.ram", ram);
}
