// Minimal regressions for already-implemented host operations, not new opcodes.
#include "sgb_host.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void file_read(void *, int, int, FileHandle *) {}
void file_write(const void *, int, int, FileHandle *) {}

static void testAbsoluteJump() {
  SgbHost host;
  host.wram[0x100] = 0x4c; // JMP $1234
  host.wram[0x101] = 0x34;
  host.wram[0x102] = 0x12;
  host.cpu.jump(0x7e0100, 0);
  host.cpu.run(host, 1);
  assert(host.cpu.state().pc == 0x1234);
  assert(host.cpu.state().pbr == 0x7e);
  // Operand fetch wraps inside the current program bank.
  host.wram[0xfffe] = 0x4c;
  host.wram[0xffff] = 0x78;
  host.wram[0] = 0x56;
  host.cpu.jump(0x7efffe, 0);
  host.cpu.run(host, 1);
  assert(host.cpu.state().pc == 0x5678);
}

static void testApuRelativeBranch() {
  SgbHostApu apu;
  apu.cpu.pc = 0x200;
  apu.ram[0x200] = 0x2f; // BRA +2, relative to end of instruction
  apu.ram[0x201] = 2;
  apu.run(1);
  assert(apu.cpu.pc == 0x204);
  apu.ram[0x204] = 0x2f;
  apu.ram[0x205] = 0xfa; // BRA -6
  apu.run(1);
  assert(apu.cpu.pc == 0x200);
  apu.cpu.pc = 0xfffe;
  apu.ram[0xfffe] = 0x2f;
  apu.ram[0xffff] = 2;
  apu.run(1);
  assert(apu.cpu.pc == 2);
}

static void testBrrSignedNibbles() {
  for (int shift = 0; shift <= 12; ++shift) {
    for (int nibble = 0; nibble < 16; ++nibble) {
      SgbHostApu apu;
      apu.voices[0].active = 1;
      apu.voices[0].brrAddress = 0x200;
      apu.dsp[3] = 0x10; // One sample advance per output sample
      apu.ram[0x200] = shift * 16; // No filter
      apu.ram[0x201] = nibble * 17; // Both nibbles have same value
      const int signedNibble = nibble < 8 ? nibble : nibble - 16;
      const int scaled = signedNibble * (1 << shift);
      const int expected = scaled >= 0 ? scaled / 2 : -((-scaled + 1) / 2);
      int16_t out;
      apu.render(&out, 1);
      assert(apu.voices[0].sample == expected);
      apu.render(&out, 1);
      assert(apu.voices[0].sample == expected);
    }
  }
}

int main(int argc, char **argv) {
  if (argc == 1 || strcmp(argv[1], "jmp") == 0) testAbsoluteJump();
  if (argc == 1 || strcmp(argv[1], "bra") == 0) testApuRelativeBranch();
  if (argc == 1 || strcmp(argv[1], "brr") == 0) testBrrSignedNibbles();
  puts("SGB baseline regressions passed");
}
