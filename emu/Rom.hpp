#pragma once

#include "Platform.hpp"
#include "Logger.hpp"

#include <vector>
#include <stdint.h>
#include <stddef.h>

struct InesRomHeader {
    char magic[4];
    uint8_t numPrgRomBanks;
    uint8_t numChrRomBanks;
    uint8_t flags6;
    uint8_t flags7;
    uint8_t numPrgRamBanks;
    uint8_t flags9;
    uint8_t flags10;
    char pad[5];

    size_t getMapper() {
        return (flags6 & 0xf) | (flags7 & 0xf0);
    }

    size_t getPrgRomSize() {
        return 16 * 1024 * numPrgRomBanks;
    }

    Word getPrgRomAccessMask() {
        return numPrgRomBanks > 1 ? 0x7fff : 0x3fff;
    }
};
static_assert(sizeof(InesRomHeader) == 16, "InesRomHeader is borked");

class Rom {
    Logger* log;
    std::vector<Byte> romData;

    int saveRamFd;
    Byte* saveRamData;

    InesRomHeader header;

    struct MapperRegs {
    } mapperRegs;

public:
    Rom(Logger* log, const char* fileName);
    ~Rom();

    void cartRomAccess(Word address, Byte* pData, bool isWrite);
    void cartRamAccess(Word address, Byte* pData, bool isWrite);
    void readRomFile(char const* fileName);
    void setupSaveRam(char const* name);
    void setupMapper();
};
