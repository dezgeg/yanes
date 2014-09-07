#include <string.h>
#include <QDebug>
#include "Sound.hpp"
#include "Utils.hpp"

static constexpr int MaxChanLevel = 0x1FFF;

void Sound::registerAccess(Word address, Byte* pData, bool isWrite) {
    if (address == 0xff15 || address == 0xff1f ||
            (address >= 0xff27 && address <= 0xff2f)) {
        log->warn("Unimplemented sound reg 0x%04x", address);
        return;
    }

    // TODO: mask
    Byte reg = address - 0xff10;
    // qDebug() << "Register write: " << reg;
    BusUtil::arrayMemAccess((Byte*)&regs, reg, pData, isWrite);

    switch (address & 0xff) {
        case Snd_Ch1_FreqHi:
        // case Snd_Ch1_Envelope:
            restartEnvelope(envelopes.ch1);
            break;

        case Snd_Ch2_FreqHi:
        // case Snd_Ch2_Envelope:
            restartEnvelope(envelopes.ch2);
            break;

        case Snd_Ch3_FreqHi:
            restartEnvelope(envelopes.ch3);
            break;

        case Snd_Ch4_Envelope:
            // TODO: need control register here
            restartEnvelope(envelopes.ch4);
            break;
    }
}

void Sound::tick(int cycleDelta) {
    currentCycle += cycleDelta;
    cycleResidue += 375 * cycleDelta;

    if (regs.ch1.square.freqCtrl.noRestart) {
        tickEnvelope(envelopes.ch1, regs.ch1.square.soundLength, 64);
    }
    if (regs.ch2.square.freqCtrl.noRestart) {
        tickEnvelope(envelopes.ch2, regs.ch2.square.soundLength, 64);
    }

    if (regs.ch3.freqCtrl.noRestart) {
        tickEnvelope(envelopes.ch3, regs.ch3.length, 256);
    }

    if (cycleResidue >= 32768) {
        cycleResidue -= 32768;
        currentSampleNumber++;

        generateSamples();
    }
}

void Sound::generateSamples() {
    int ch1Volume = evalPulseChannel(regs.ch1.square, envelopes.ch1);
    int ch2Volume = evalPulseChannel(regs.ch2.square, envelopes.ch2);
    int ch3Volume = evalWaveChannel();

    // qDebug() << ch1Volume << ch2Volume << ch3Volume;

    rightSample = ch1Volume + ch2Volume + ch3Volume;
    leftSample = rightSample;
}

int Sound::evalWaveChannel() {
    if (!envelopes.ch3.startCycle || !regs.ch3.enable) {
        return 0;
    }

    static const unsigned shiftLookup[] = { 4, 0, 1, 2 };

    // TODO: make some sense of this code
    unsigned period = (2048 - regs.ch3.freqCtrl.getFrequency());
    unsigned cycleInWave = (currentCycle - envelopes.ch3.startCycle) % (period * 32 * 2);
    unsigned pointer = ((cycleInWave / period) / 2 + 1) % 32;

    unsigned sample = (regs.waveRam[pointer >> 1] >> (pointer ? 4 : 0)) & 0xf;
    sample >>= shiftLookup[regs.ch3.volume];
    // qDebug() << "Wave sample: " << sample << " at pointer: " << ((cycleInWave / period) / 2 + 1);

    return (sample - 8) * (MaxChanLevel / 8);
}

int Sound::evalPulseChannel(SquareChannelRegs& regs, EnvelopeState& envelState) {
    if (!envelState.startCycle) {
        return 0;
    }

    bool pulseOn = evalPulseWaveform(regs);
    unsigned pulseVolume = evalEnvelope(regs.envelope, envelState);
    // qDebug() << "Envel result: " << pulseVolume;
    return mixVolume(pulseOn ? MaxChanLevel : -MaxChanLevel, pulseVolume);
}

// Returns true/false whether the square channel output is high or low
bool Sound::evalPulseWaveform(SquareChannelRegs& ch) {
    static const unsigned pulseWidthLookup[] = { 4 * 1, 4 * 2, 4 * 4, 4 * 6 };

    unsigned pulseLen = 2048 - ch.freqCtrl.getFrequency();
    unsigned long stepInPulse = currentCycle % (pulseLen * 4 * 8);

    return stepInPulse / pulseLen < pulseWidthLookup[ch.waveDuty];
}

void Sound::tickEnvelope(EnvelopeState& state, unsigned curLength, unsigned channelMaxLength) {
    if ((currentCycle - state.startCycle) >> 16 > (channelMaxLength - curLength)) {
        state.startCycle = 0;
    }
}

void Sound::restartEnvelope(EnvelopeState& state) {
    // qDebug() << "Restart envelope at cycle " << currentCycle;
    state.startCycle = currentCycle;
}

unsigned int Sound::evalEnvelope(EnvelopeRegs& regs, EnvelopeState& state) {
    if (regs.sweep == 0) {
        return regs.initialVolume;
    }

    long steps = (currentCycle - state.startCycle) / ((1 << 16) * regs.sweep);
    // qDebug() << "Envelope steps: " << steps << " at cycle " << currentCycle;
    return clamp(regs.initialVolume + (regs.increase ? steps : -steps), 0, 0xf);
}

int Sound::mixVolume(int sample, unsigned int volume) {
    return sample * (int)volume / 0xf;
}

Sound::Sound(Logger* log) : log(log),
                            currentCycle(),
                            cycleResidue(),
                            currentSampleNumber(),
                            leftSample(),
                            rightSample() {
    memset(&regs, 0, sizeof(regs));
    memset(&envelopes, 0, sizeof(envelopes));
}
