#pragma once

#include "Cpu.hpp"
#include "Gpu.hpp"
#include "Joypad.hpp"
#include "Logger.hpp"
#include "Rom.hpp"
#include "Serial.hpp"
#include "Sound.hpp"

class Nes {
    Logger* log;
    Bus bus;
    Gpu gpu;
    Cpu cpu;
    Joypad joypad;
    Serial serial;
    Sound sound;
    long currentCycle;

public:
    Nes(Logger* log, Rom* rom) :
            log(log),
            bus(log, rom, &gpu, &joypad, &serial, &sound),
            gpu(log),
            cpu(log, &bus),
            joypad(),
            serial(),
            sound(log),
            currentCycle(0) {
    }

    Bus* getBus() { return &bus; }
    Cpu* getCpu() { return &cpu; }
    Gpu* getGpu() { return &gpu; }
    Sound* getSound() { return &sound; }
    Joypad* getJoypad() { return &joypad; }

    void runOneInstruction();
};
