#include "BusUtil.hpp"
#include "Logger.hpp"
#include "Rom.hpp"
#include "Utils.hpp"

#include <fstream>
#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

static constexpr size_t MAX_SAVE_RAM_SIZE = 0x10000;

Rom::Rom(Logger* log, const char* fileName) :
        log(log),
        saveRamFd(-1),
        saveRamData((Byte*)MAP_FAILED) {

    readRomFile(fileName);
    setupSaveRam(fileName);
    setupMapper();
}

void Rom::readRomFile(char const* fileName) {
    std::ifstream stream(fileName, std::ios_base::in | std::ios_base::binary);
    stream.seekg(0, std::ios_base::end);
    if (!stream) {
        throw "No such file";
    }

    size_t sz = stream.tellg();
    stream.seekg(0, std::ios_base::beg);
    if (sz < sizeof(InesRomHeader)) {
        throw "Rom file too small";
    }
    stream.read((char*)&header, sizeof(InesRomHeader));

    log->warn("INES header - mapper %d, PRG banks: %d, CHR ROM banks: %d",
              header.getMapper(), header.numPrgRomBanks, header.numChrRomBanks);

    prgRomData.resize(header.getPrgRomSize());
    stream.read((char*)&prgRomData[0], header.getPrgRomSize());
    chrRomData.resize(std::max(8192ul, header.getChrRomSize()));
    stream.read((char*)&chrRomData[0], header.getChrRomSize());

    assert(sz == (size_t)stream.tellg());
}

void Rom::setupSaveRam(char const* name) {
    std::string saveRamFile = name;
    size_t dotIndex = saveRamFile.find_last_of('.');
    if (dotIndex != std::string::npos) {
        saveRamFile = saveRamFile.substr(0, dotIndex);
    }
    saveRamFile += ".sav";

    saveRamFd = open(saveRamFile.c_str(), O_CREAT | O_RDWR, 0644);
    if (saveRamFd < 0) {
        throw "Can't open save RAM file";
    }
    if (ftruncate(saveRamFd, MAX_SAVE_RAM_SIZE) < 0) {
        throw "Can't resize save RAM file";
    }
    saveRamData = (Byte*)mmap(nullptr, MAX_SAVE_RAM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, saveRamFd, 0);
    if (saveRamData == MAP_FAILED) {
        throw "Can't mmap save RAM file";
    }
}

void Rom::setupMapper() {
    std::memset(&mapperRegs, 0, sizeof(mapperRegs));
}

void Rom::cartRomAccess(Word address, Byte* pData, bool isWrite) {
    address &= header.getPrgRomAccessMask();
    if (address >= header.getPrgRomSize()) {
        log->warn("Accessing nonexistent ROM: 0x%04x, max 0x%04x", address, header.getPrgRomSize());
    }

    if (isWrite) {
        log->warn("ROM write to address 0x%04x", address);
    } else {
        BusUtil::arrayMemAccess(prgRomData.data(), address, pData, isWrite);
    }
}

void Rom::cartRamAccess(Word address, Byte* pData, bool isWrite) {
    BusUtil::arrayMemAccess(saveRamData, address, pData, isWrite);
}

Rom::~Rom() {
    if (saveRamData != MAP_FAILED) {
        munmap(saveRamData, MAX_SAVE_RAM_SIZE);
    }
    if (saveRamFd != -1) {
        close(saveRamFd);
    }
}
