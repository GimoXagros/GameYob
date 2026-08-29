#pragma once

#include <stddef.h>
#include <stdint.h>

struct FileHandle;

class SgbHost;

class SgbHostCpu {
public:
  struct State {
    uint16_t a, x, y, sp, d, pc;
    uint8_t pbr, dbr, p;
    uint8_t emulation, waiting, stopped, faulted, faultOpcode;
    uint32_t nmiVector;
  };

  SgbHostCpu();
  void reset();
  void jump(uint32_t address, uint32_t nmiAddress);
  void requestNmi();
  int run(SgbHost &host, int instructionBudget);
  const State &state() const { return cpu; }
  State &state() { return cpu; }

private:
  State cpu;
  bool nmiPending;
};

class SgbHostPpu {
public:
  struct ObjectPixel {
    uint16_t color;
    uint8_t priority;
    uint8_t visible;
  };

  SgbHostPpu();
  void reset();
  uint8_t readRegister(uint16_t address);
  void writeRegister(uint16_t address, uint8_t value);
  void loadCharacterData(const uint8_t *data, uint8_t destination);
  void loadPrototypeObjects(const uint8_t *data, size_t length,
                            const uint8_t *systemPalettes,
                            const uint16_t paletteIds[4]);
  ObjectPixel objectPixel(int x, int y) const;

  uint8_t vram[0x10000];
  uint8_t cgram[0x200];
  uint8_t oam[0x220];
  uint16_t vramAddress;
  uint16_t cgramAddress;
  uint16_t oamAddress;
  uint8_t objectControl;
  uint8_t objectEnabled;
};

class SgbHostApu {
public:
  struct CpuState {
    uint16_t pc, sp;
    uint8_t a, x, y, psw;
    uint8_t stopped, faulted, faultOpcode;
  };

  struct VoiceState {
    uint32_t phase;
    uint16_t brrAddress;
    uint8_t nibble;
    int16_t previous1, previous2, sample;
    uint8_t active;
  };

  SgbHostApu();
  void reset();
  bool transferProgram(const uint8_t *data, size_t length);
  void applySoundCommand(const uint8_t *command, size_t length);
  int run(int instructionBudget);
  void render(int16_t *output, size_t samples);
  bool hasActiveAudio() const;

  uint8_t ram[0x10000];
  uint8_t dsp[0x80];
  uint8_t dspAddress;
  uint8_t inputPorts[4];
  CpuState cpu;
  VoiceState voices[8];

private:
  uint8_t read(uint16_t address) const;
  void write(uint16_t address, uint8_t value);
  int16_t decodeBrrSample(int voice);
  void keyOn(uint8_t mask);
};

class SgbHost {
public:
  enum { STATE_FORMAT = 1 };

  SgbHost();
  void reset();
  bool writeMemory(uint32_t address, const uint8_t *data, size_t length);
  uint8_t read8(uint32_t address);
  void write8(uint32_t address, uint8_t value);
  void jump(uint32_t address, uint32_t nmiAddress);
  void runFrame();
  void renderAudio(int16_t *output, size_t samples);
  bool saveState(FileHandle *file) const;
  bool loadState(FileHandle *file);

  SgbHostCpu cpu;
  SgbHostPpu ppu;
  SgbHostApu apu;
  uint8_t wram[0x20000];
};
