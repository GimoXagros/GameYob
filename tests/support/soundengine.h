#pragma once

// Test-only platform boundary. The lifecycle harness exercises production
// Gameboy/MMU/RomFile logic, but not audio output hardware.
class Gameboy;
#define CHAN_1 1
#define CHAN_2 2
#define CHAN_3 4
#define CHAN_4 8
void unmuteSND();
class SoundEngine {
public:
    explicit SoundEngine(Gameboy*) : cyclesToSoundEvent(10000) {}
    void init() {}
    void refresh() {}
    void mute() {}
    void updateSound(int) {}
    void soundUpdateVBlank() {}
    void handleSoundRegister(unsigned char, unsigned char) {}
    int cyclesToSoundEvent;
};
