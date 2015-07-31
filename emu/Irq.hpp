#pragma once

#include "Platform.hpp"
#include "Utils.hpp"

enum Irq {
    Irq_None,

    Irq_NMI,
    Irq_IRQ
};

typedef Byte IrqSet;
