#pragma once

#include "Cpu.hpp"
#include "Gpu.hpp"
#include "Joypad.hpp"
#include "Logger.hpp"
#include "Rom.hpp"
#include "Sound.hpp"

class Nes {
    Logger* log;
    Bus bus;
    Gpu gpu;
    Cpu cpu;
    Joypad joypad;
    Sound sound;
    long currentCycle;

public:
    Nes(Logger* log, Rom* rom) :
            log(log),
            bus(log, rom, &gpu, &joypad, &sound),
            gpu(log, &bus),
            cpu(log, &bus),
            joypad(log),
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
