#include "sgb_host.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// State I/O is covered by the emulator integration build. Portable host tests
// do not open files, but provide these symbols so the core remains standalone.
void file_read(void *, int, int, FileHandle *) {}
void file_write(const void *, int, int, FileHandle *) {}

static void testHostMemoryAndCpu() {
  SgbHost host;
  const uint8_t program[] = {
      0xa9, 0x42,             // LDA #$42
      0x8f, 0x10, 0x00, 0x7e, // STA $7e0010
      0xdb                    // STP
  };
  assert(host.writeMemory(0x7e0000, program, sizeof(program)));
  assert(!host.writeMemory(0x400000, program, sizeof(program)));
  host.jump(0x7e0000, 0);
  assert(host.wram[0x10] == 0x42);
  assert(host.cpu.state().stopped);
  assert(!host.cpu.state().faulted);
}

static void testSoftwareInterruptOpcode() {
  SgbHost host;
  const uint8_t program[] = {0x02, 0x00};
  assert(host.writeMemory(0x7e0000, program, sizeof(program)));
  host.cpu.jump(0x7e0000, 0);
  assert(host.cpu.run(host, 1) == 1);
  assert(!host.cpu.state().faulted);
  assert(host.cpu.state().pc == 0xffff);
  assert(host.cpu.state().pbr == 0);
}

static void testApuProgramTransfer() {
  SgbHostApu apu;
  const uint8_t transfer[] = {3,    0, 0x00, 0x02, 0xe8, 0x7f,
                              0xff, 0, 0,    0x00, 0x02};
  assert(apu.transferProgram(transfer, sizeof(transfer)));
  apu.run(8);
  assert(apu.cpu.a == 0x7f);
  assert(apu.cpu.stopped);
  assert(!apu.cpu.faulted);
}

static void testPrototypeObjectDecode() {
  SgbHostPpu ppu;
  ppu.objectEnabled = 1;
  ppu.oam[0] = 10;
  ppu.oam[1] = 20;
  ppu.oam[2] = 0;
  ppu.oam[3] = 8; // OBJ palette 4
  ppu.vram[0] = 0x80;
  ppu.cgram[4 * 32 + 2] = 0x34;
  ppu.cgram[4 * 32 + 3] = 0x12;
  SgbHostPpu::ObjectPixel pixel = ppu.objectPixel(10, 20);
  assert(pixel.visible);
  assert(pixel.color == 0x1234);
  assert(!ppu.objectPixel(11, 20).visible);
}

int main() {
  testHostMemoryAndCpu();
  testSoftwareInterruptOpcode();
  testApuProgramTransfer();
  testPrototypeObjectDecode();
  puts("SGB host tests passed");
  return 0;
}
