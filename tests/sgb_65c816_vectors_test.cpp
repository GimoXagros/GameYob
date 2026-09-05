#include "sgb_host.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void file_read(void *, int, int, FileHandle *) {}
void file_write(const void *, int, int, FileHandle *) {}

enum { C = 0x01, Z = 0x02, X = 0x10, M = 0x20, V = 0x40, N = 0x80 };

static void instruction(SgbHost &host, const uint8_t *bytes, size_t size) {
  memset(host.wram, 0, 16);
  memcpy(host.wram, bytes, size);
  host.cpu.jump(0x7e0000, 0);
  assert(host.cpu.run(host, 1) == 1);
  assert(!host.cpu.state().faulted);
}

static void native16(SgbHost &host) {
  const uint8_t xce[] = {0xfb};
  instruction(host, xce, sizeof(xce));
  assert(!host.cpu.state().emulation);
  const uint8_t rep[] = {0xc2, 0x30};
  instruction(host, rep, sizeof(rep));
  assert(!(host.cpu.state().p & (M | X)));
}

static void testImmediateLogicAndCompare() {
  struct Logic { uint8_t op, initial, operand, result; } cases[] = {
      {0x09, 0x50, 0x0f, 0x5f}, {0x29, 0x5a, 0x0f, 0x0a},
      {0x49, 0x55, 0xff, 0xaa}};
  for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    SgbHost host;
    host.cpu.state().a = 0xab00 | cases[i].initial;
    const uint8_t code[] = {cases[i].op, cases[i].operand};
    instruction(host, code, sizeof(code));
    assert(host.cpu.state().a == (uint16_t)(0xab00 | cases[i].result));
    assert(!!(host.cpu.state().p & N) == !!(cases[i].result & 0x80));
    assert(!!(host.cpu.state().p & Z) == (cases[i].result == 0));
  }
  SgbHost bit;
  bit.cpu.state().a = 0x55;
  const uint8_t bitCode[] = {0x89, 0xaa};
  instruction(bit, bitCode, sizeof(bitCode));
  assert(bit.cpu.state().a == 0x55 && (bit.cpu.state().p & Z));

  struct Compare { uint8_t op, lhs, rhs; bool carry, zero, negative; } cmp[] = {
      {0xc9, 5, 5, true, true, false},
      {0xe0, 1, 2, false, false, true},
      {0xc0, 3, 2, true, false, false}};
  for (unsigned i = 0; i < sizeof(cmp) / sizeof(cmp[0]); ++i) {
    SgbHost host;
    host.cpu.state().a = cmp[i].lhs;
    host.cpu.state().x = cmp[i].lhs;
    host.cpu.state().y = cmp[i].lhs;
    const uint8_t code[] = {cmp[i].op, cmp[i].rhs};
    instruction(host, code, sizeof(code));
    assert(!!(host.cpu.state().p & C) == cmp[i].carry);
    assert(!!(host.cpu.state().p & Z) == cmp[i].zero);
    assert(!!(host.cpu.state().p & N) == cmp[i].negative);
  }
}

static void testAccumulatorShifts() {
  struct Shift { uint8_t op, input, flags, output; } cases[] = {
      {0x0a, 0x81, 0, 0x02}, {0x4a, 0x01, 0, 0x00},
      {0x2a, 0x80, C, 0x01}, {0x6a, 0x01, C, 0x80}};
  for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    SgbHost host;
    host.cpu.state().a = 0x7e00 | cases[i].input;
    host.cpu.state().p = (host.cpu.state().p & ~C) | cases[i].flags;
    instruction(host, &cases[i].op, 1);
    assert((host.cpu.state().a & 0xff) == cases[i].output);
    assert((host.cpu.state().a >> 8) == 0x7e);
    assert(!!(host.cpu.state().p & C) == !!(cases[i].input &
        (cases[i].op == 0x0a || cases[i].op == 0x2a ? 0x80 : 1)));
  }
}

static void testWidthsTransfersAndStack() {
  SgbHost host;
  native16(host);
  host.cpu.state().x = 0x1234;
  const uint8_t txa[] = {0x8a};
  instruction(host, txa, sizeof(txa));
  assert(host.cpu.state().a == 0x1234);
  const uint8_t sep[] = {0xe2, X};
  instruction(host, sep, sizeof(sep));
  assert(host.cpu.state().x == 0x34);

  host.cpu.state().a = 0x8000;
  const uint8_t tcd[] = {0x5b};
  instruction(host, tcd, sizeof(tcd));
  assert(host.cpu.state().d == 0x8000 && (host.cpu.state().p & N));
  const uint8_t phd[] = {0x0b};
  instruction(host, phd, sizeof(phd));
  host.cpu.state().d = 0;
  const uint8_t pld[] = {0x2b};
  instruction(host, pld, sizeof(pld));
  assert(host.cpu.state().d == 0x8000);

  host.cpu.state().a = 0x1234;
  const uint8_t xba[] = {0xeb};
  instruction(host, xba, sizeof(xba));
  assert(host.cpu.state().a == 0x3412);
}

static void testLoadStoreAddressing() {
  SgbHost host;
  host.cpu.state().d = 0x0100;
  host.cpu.state().x = 3;
  host.wram[0x0123] = 0x5a;
  const uint8_t ldaDirectX[] = {0xb5, 0x20};
  instruction(host, ldaDirectX, sizeof(ldaDirectX));
  assert((host.cpu.state().a & 0xff) == 0x5a);

  host.cpu.state().dbr = 0x7e;
  host.cpu.state().y = 2;
  host.wram[0x0202] = 0xa5;
  const uint8_t ldaAbsoluteY[] = {0xb9, 0x00, 0x02};
  instruction(host, ldaAbsoluteY, sizeof(ldaAbsoluteY));
  assert((host.cpu.state().a & 0xff) == 0xa5);

  host.cpu.state().a = 0x77;
  const uint8_t staLongX[] = {0x9f, 0xfd, 0x02, 0x7e};
  instruction(host, staLongX, sizeof(staLongX));
  assert(host.wram[0x0300] == 0x77);

  host.cpu.state().x = 4;
  host.wram[0x0144] = 0x66;
  const uint8_t ldyDirectX[] = {0xb4, 0x40};
  instruction(host, ldyDirectX, sizeof(ldyDirectX));
  assert(host.cpu.state().y == 0x66);

  native16(host);
  host.cpu.state().dbr = 0x7e;
  host.cpu.state().x = 2;
  host.wram[0x0302] = 0x34;
  host.wram[0x0303] = 0x12;
  const uint8_t ldaAbsoluteX[] = {0xbd, 0x00, 0x03};
  instruction(host, ldaAbsoluteX, sizeof(ldaAbsoluteX));
  assert(host.cpu.state().a == 0x1234);

  SgbHost indexedBankCross;
  indexedBankCross.cpu.state().dbr = 0x7e;
  indexedBankCross.cpu.state().x = 2;
  indexedBankCross.wram[0x10000] = 0x5a;
  const uint8_t ldaAbsoluteXCross[] = {0xbd, 0xfe, 0xff};
  instruction(indexedBankCross, ldaAbsoluteXCross,
              sizeof(ldaAbsoluteXCross));
  assert((indexedBankCross.cpu.state().a & 0xff) == 0x5a);

  SgbHost wordBankCross;
  native16(wordBankCross);
  wordBankCross.cpu.state().dbr = 0x7e;
  wordBankCross.wram[0xffff] = 0x34;
  wordBankCross.wram[0x10000] = 0x12;
  const uint8_t ldaAbsoluteCross[] = {0xad, 0xff, 0xff};
  instruction(wordBankCross, ldaAbsoluteCross, sizeof(ldaAbsoluteCross));
  assert(wordBankCross.cpu.state().a == 0x1234);

  SgbHost emulationDirectWrap;
  emulationDirectWrap.cpu.state().d = 0x0100;
  emulationDirectWrap.cpu.state().x = 2;
  emulationDirectWrap.wram[0x0101] = 0x5a;
  emulationDirectWrap.wram[0x0201] = 0xa5;
  const uint8_t ldaDirectXWrap[] = {0xb5, 0xff};
  instruction(emulationDirectWrap, ldaDirectXWrap, sizeof(ldaDirectXWrap));
  assert((emulationDirectWrap.cpu.state().a & 0xff) == 0x5a);

  SgbHost nativeDirectCross;
  native16(nativeDirectCross);
  nativeDirectCross.cpu.state().d = 0x0100;
  nativeDirectCross.cpu.state().x = 2;
  nativeDirectCross.wram[0x0201] = 0xa5;
  instruction(nativeDirectCross, ldaDirectXWrap, sizeof(ldaDirectXWrap));
  assert(nativeDirectCross.cpu.state().a == 0x00a5);
}

static uint8_t bcd(int value) { return (uint8_t)((value / 10) * 16 + value % 10); }

static void testImmediateArithmetic() {
  for (int decimal = 0; decimal < 2; ++decimal) {
    for (int left = 0; left < 100; ++left) {
      for (int right = 0; right < 100; ++right) {
        for (int carry = 0; carry < 2; ++carry) {
          SgbHost add;
          add.cpu.state().a = 0x5a00 | bcd(left);
          add.cpu.state().p = (add.cpu.state().p & ~(C | 0x08)) |
              (carry ? C : 0) | (decimal ? 0x08 : 0);
          const uint8_t adc[] = {0x69, bcd(right)};
          instruction(add, adc, sizeof(adc));
          int raw = bcd(left) + bcd(right) + carry;
          int expected = decimal ? (left + right + carry) : raw;
          const uint8_t expectedByte = decimal ? bcd(expected % 100) : expected;
          assert((add.cpu.state().a & 0xff) == expectedByte);
          assert((add.cpu.state().a >> 8) == 0x5a);
          assert(!!(add.cpu.state().p & C) ==
              (decimal ? expected >= 100 : raw >= 256));

          SgbHost subtract;
          subtract.cpu.state().a = 0xa500 | bcd(left);
          subtract.cpu.state().p = (subtract.cpu.state().p & ~(C | 0x08)) |
              (carry ? C : 0) | (decimal ? 0x08 : 0);
          const uint8_t sbc[] = {0xe9, bcd(right)};
          instruction(subtract, sbc, sizeof(sbc));
          int difference = (decimal ? left : bcd(left)) -
              (decimal ? right : bcd(right)) - (carry ? 0 : 1);
          int wrapped = decimal ? ((difference % 100) + 100) % 100 : difference & 0xff;
          const uint8_t expectedSubtract = decimal ? bcd(wrapped) : wrapped;
          assert((subtract.cpu.state().a & 0xff) == expectedSubtract);
          assert((subtract.cpu.state().a >> 8) == 0xa5);
          assert(!!(subtract.cpu.state().p & C) == (difference >= 0));
        }
      }
    }
  }
  SgbHost overflow;
  overflow.cpu.state().a = 0x7f;
  overflow.cpu.state().p &= ~(C | 0x08);
  const uint8_t adc[] = {0x69, 1};
  instruction(overflow, adc, sizeof(adc));
  assert((overflow.cpu.state().p & V) && (overflow.cpu.state().p & N));

  SgbHost wide;
  native16(wide);
  wide.cpu.state().a = 0x9999;
  wide.cpu.state().p |= C | 0x08;
  const uint8_t adc16[] = {0x69, 0x00, 0x00};
  instruction(wide, adc16, sizeof(adc16));
  assert(wide.cpu.state().a == 0 && (wide.cpu.state().p & C));
}

static void testMemoryShiftAndUpdateFamilies() {
  struct RmwCase {
    uint8_t op;
    bool absolute;
    bool indexed;
    uint8_t input;
    bool carryIn;
    uint8_t result;
    bool carryOut;
  } cases[] = {
      {0x06, false, false, 0x81, false, 0x02, true},
      {0x16, false, true,  0x40, false, 0x80, false},
      {0x0e, true,  false, 0x80, false, 0x00, true},
      {0x1e, true,  true,  0x01, false, 0x02, false},
      {0x26, false, false, 0x80, true,  0x01, true},
      {0x36, false, true,  0x40, false, 0x80, false},
      {0x2e, true,  false, 0xff, false, 0xfe, true},
      {0x3e, true,  true,  0x00, true,  0x01, false},
      {0x46, false, false, 0x01, false, 0x00, true},
      {0x56, false, true,  0x80, false, 0x40, false},
      {0x4e, true,  false, 0xff, false, 0x7f, true},
      {0x5e, true,  true,  0x02, false, 0x01, false},
      {0x66, false, false, 0x01, true,  0x80, true},
      {0x76, false, true,  0x80, false, 0x40, false},
      {0x6e, true,  false, 0x02, true,  0x81, false},
      {0x7e, true,  true,  0xff, false, 0x7f, true},
  };
  for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    SgbHost host;
    host.cpu.state().d = 0x0100;
    host.cpu.state().dbr = 0x7e;
    host.cpu.state().x = cases[i].indexed ? 2 : 0;
    const uint16_t address = cases[i].absolute ?
        uint16_t(0x0220 + host.cpu.state().x) :
        uint16_t(0x0120 + host.cpu.state().x);
    host.wram[address] = cases[i].input;
    host.cpu.state().p = (host.cpu.state().p & ~C) |
        (cases[i].carryIn ? C : 0);
    const uint8_t code[] = {cases[i].op, 0x20, 0x02};
    instruction(host, code, cases[i].absolute ? 3 : 2);
    assert(host.wram[address] == cases[i].result);
    assert(!!(host.cpu.state().p & C) == cases[i].carryOut);
    assert(!!(host.cpu.state().p & Z) == (cases[i].result == 0));
    assert(!!(host.cpu.state().p & N) == !!(cases[i].result & 0x80));
  }

  const uint8_t updateOps[] = {0xc6, 0xd6, 0xce, 0xde,
                               0xe6, 0xf6, 0xee, 0xfe};
  for (unsigned i = 0; i < sizeof(updateOps); ++i) {
    const bool increment = updateOps[i] >= 0xe0;
    const bool absolute = (updateOps[i] & 0x08) != 0;
    const bool indexed = (updateOps[i] & 0x10) != 0;
    SgbHost host;
    host.cpu.state().d = 0x0100;
    host.cpu.state().dbr = 0x7e;
    host.cpu.state().x = indexed ? 2 : 0;
    const uint16_t address = absolute ?
        uint16_t(0x0220 + host.cpu.state().x) :
        uint16_t(0x0120 + host.cpu.state().x);
    host.wram[address] = increment ? 0xff : 0;
    const uint8_t code[] = {updateOps[i], 0x20, 0x02};
    instruction(host, code, absolute ? 3 : 2);
    assert(host.wram[address] == (increment ? 0 : 0xff));
    assert(!!(host.cpu.state().p & Z) == increment);
    assert(!!(host.cpu.state().p & N) != increment);
  }
}

static void testMemoryBitFamilies() {
  struct BitCase { uint8_t op; bool absolute; bool indexed; } bitCases[] = {
      {0x24, false, false}, {0x34, false, true},
      {0x2c, true, false}, {0x3c, true, true}};
  for (unsigned i = 0; i < sizeof(bitCases) / sizeof(bitCases[0]); ++i) {
    SgbHost host;
    host.cpu.state().a = 0x0f;
    host.cpu.state().d = 0x0100;
    host.cpu.state().dbr = 0x7e;
    host.cpu.state().x = bitCases[i].indexed ? 2 : 0;
    const uint16_t address = bitCases[i].absolute ?
        uint16_t(0x0220 + host.cpu.state().x) :
        uint16_t(0x0120 + host.cpu.state().x);
    host.wram[address] = 0xc0;
    const uint8_t code[] = {bitCases[i].op, 0x20, 0x02};
    instruction(host, code, bitCases[i].absolute ? 3 : 2);
    assert(host.cpu.state().p & Z);
    assert(host.cpu.state().p & N);
    assert(host.cpu.state().p & V);
  }

  struct ChangeCase { uint8_t op; bool absolute; uint8_t expected; } changes[] = {
      {0x04, false, 0xff}, {0x0c, true, 0xff},
      {0x14, false, 0xf0}, {0x1c, true, 0xf0}};
  for (unsigned i = 0; i < sizeof(changes) / sizeof(changes[0]); ++i) {
    SgbHost host;
    host.cpu.state().a = 0x0f;
    host.cpu.state().d = 0x0100;
    host.cpu.state().dbr = 0x7e;
    const uint16_t address = changes[i].absolute ? 0x0220 : 0x0120;
    host.wram[address] = 0xf0;
    const uint8_t code[] = {changes[i].op, 0x20, 0x02};
    instruction(host, code, changes[i].absolute ? 3 : 2);
    assert(host.cpu.state().p & Z);
    assert(host.wram[address] == changes[i].expected);
  }

  SgbHost wide;
  native16(wide);
  wide.cpu.state().a = 0x00ff;
  wide.cpu.state().dbr = 0x7e;
  wide.wram[0x02ff] = 0xff;
  wide.wram[0x0300] = 0x40;
  const uint8_t trb16[] = {0x1c, 0xff, 0x02};
  instruction(wide, trb16, sizeof(trb16));
  assert(wide.wram[0x02ff] == 0 && wide.wram[0x0300] == 0x40);
  assert(!(wide.cpu.state().p & Z));

  SgbHost wideShift;
  native16(wideShift);
  wideShift.cpu.state().dbr = 0x7e;
  wideShift.wram[0x02ff] = 0x01;
  wideShift.wram[0x0300] = 0x80;
  const uint8_t asl16[] = {0x0e, 0xff, 0x02};
  instruction(wideShift, asl16, sizeof(asl16));
  assert(wideShift.wram[0x02ff] == 0x02);
  assert(wideShift.wram[0x0300] == 0x00);
  assert(wideShift.cpu.state().p & C);
  assert(!(wideShift.cpu.state().p & (Z | N)));
}

static uint16_t prepareIndirectAddress(SgbHost &host, int mode) {
  host.cpu.state().d = 0x0100;
  host.cpu.state().dbr = 0x7e;
  host.cpu.state().x = mode == 0 ? 2 : 0;
  host.cpu.state().y = (mode == 3 || mode == 5 || mode == 6) ? 2 : 0;
  if (mode == 1)
    return uint16_t(host.cpu.state().sp + 0x20);
  const uint16_t pointerAddress = mode == 5 ?
      uint16_t(host.cpu.state().sp + 0x20) :
      uint16_t(0x0120 + (mode == 0 ? host.cpu.state().x : 0));
  const uint16_t pointer = (mode == 3 || mode == 5 || mode == 6) ?
      0x02fe : 0x0300;
  host.wram[pointerAddress] = pointer & 0xff;
  host.wram[uint16_t(pointerAddress + 1)] = pointer >> 8;
  if (mode == 2 || mode == 6)
    host.wram[uint16_t(pointerAddress + 2)] = 0x7e;
  return uint16_t(pointer + host.cpu.state().y);
}

static void testIndirectLoadStoreAddressing() {
  struct IndirectCase { uint8_t load, store; int mode; } cases[] = {
      {0xa1, 0x81, 0}, // (dp,X)
      {0xa3, 0x83, 1}, // stack relative
      {0xa7, 0x87, 2}, // [dp]
      {0xb1, 0x91, 3}, // (dp),Y
      {0xb2, 0x92, 4}, // (dp)
      {0xb3, 0x93, 5}, // (stack,S),Y
      {0xb7, 0x97, 6}, // [dp],Y
  };
  for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    SgbHost load;
    const uint16_t loadAddress = prepareIndirectAddress(load, cases[i].mode);
    load.wram[loadAddress] = 0x5a;
    const uint8_t loadCode[] = {cases[i].load, 0x20};
    instruction(load, loadCode, sizeof(loadCode));
    assert((load.cpu.state().a & 0xff) == 0x5a);

    SgbHost store;
    const uint16_t storeAddress = prepareIndirectAddress(store, cases[i].mode);
    store.cpu.state().a = 0xa5;
    const uint8_t storeCode[] = {cases[i].store, 0x20};
    instruction(store, storeCode, sizeof(storeCode));
    assert(store.wram[storeAddress] == 0xa5);
  }

  SgbHost longBoundary;
  native16(longBoundary);
  longBoundary.cpu.state().d = 0x0100;
  longBoundary.wram[0x0120] = 0xff;
  longBoundary.wram[0x0121] = 0xff;
  longBoundary.wram[0x0122] = 0x7e;
  longBoundary.wram[0xffff] = 0x34;
  longBoundary.wram[0x10000] = 0x12;
  const uint8_t ldaLongIndirect[] = {0xa7, 0x20};
  instruction(longBoundary, ldaLongIndirect, sizeof(ldaLongIndirect));
  assert(longBoundary.cpu.state().a == 0x1234);

  SgbHost indexedBankCross;
  indexedBankCross.cpu.state().d = 0x0100;
  indexedBankCross.cpu.state().dbr = 0x7e;
  indexedBankCross.cpu.state().y = 2;
  indexedBankCross.wram[0x0120] = 0xfe;
  indexedBankCross.wram[0x0121] = 0xff;
  indexedBankCross.wram[0x10000] = 0xa5;
  const uint8_t ldaIndirectYCross[] = {0xb1, 0x20};
  instruction(indexedBankCross, ldaIndirectYCross,
              sizeof(ldaIndirectYCross));
  assert((indexedBankCross.cpu.state().a & 0xff) == 0xa5);

  SgbHost indirectPageWrap;
  indirectPageWrap.cpu.state().d = 0x0100;
  indirectPageWrap.cpu.state().dbr = 0x7e;
  indirectPageWrap.wram[0x01ff] = 0x00;
  indirectPageWrap.wram[0x0100] = 0x03;
  indirectPageWrap.wram[0x0200] = 0x04;
  indirectPageWrap.wram[0x0300] = 0x77;
  const uint8_t ldaIndirectWrap[] = {0xb2, 0xff};
  instruction(indirectPageWrap, ldaIndirectWrap, sizeof(ldaIndirectWrap));
  assert((indirectPageWrap.cpu.state().a & 0xff) == 0x77);

  SgbHost longIndirectCross;
  longIndirectCross.cpu.state().d = 0x0100;
  longIndirectCross.wram[0x01ff] = 0x00;
  longIndirectCross.wram[0x0200] = 0x03;
  longIndirectCross.wram[0x0201] = 0x7e;
  longIndirectCross.wram[0x0300] = 0x88;
  const uint8_t ldaLongIndirectCross[] = {0xa7, 0xff};
  instruction(longIndirectCross, ldaLongIndirectCross,
              sizeof(ldaLongIndirectCross));
  assert((longIndirectCross.cpu.state().a & 0xff) == 0x88);
}

static uint16_t prepareMemoryOperand(SgbHost &host, int mode,
                                     uint8_t code[4], size_t &length) {
  code[1] = 0x20;
  code[2] = 0x02;
  code[3] = 0x7e;
  length = 2;
  switch (mode) {
  case 0: return prepareIndirectAddress(host, 0); // (dp,X)
  case 1: return prepareIndirectAddress(host, 1); // stack relative
  case 2:
    host.cpu.state().d = 0x0100;
    return 0x0120;
  case 3: return prepareIndirectAddress(host, 2); // [dp]
  case 4:
    host.cpu.state().dbr = 0x7e;
    length = 3;
    return 0x0220;
  case 5:
    length = 4;
    return 0x0220;
  case 6: return prepareIndirectAddress(host, 3); // (dp),Y
  case 7: return prepareIndirectAddress(host, 4); // (dp)
  case 8: return prepareIndirectAddress(host, 5); // (stack,S),Y
  case 9:
    host.cpu.state().d = 0x0100;
    host.cpu.state().x = 2;
    return 0x0122;
  case 10: return prepareIndirectAddress(host, 6); // [dp],Y
  case 11:
    host.cpu.state().dbr = 0x7e;
    host.cpu.state().y = 2;
    length = 3;
    return 0x0222;
  case 12:
    host.cpu.state().dbr = 0x7e;
    host.cpu.state().x = 2;
    length = 3;
    return 0x0222;
  default:
    host.cpu.state().x = 2;
    length = 4;
    return 0x0222;
  }
}

static void testMemoryArithmeticAddressing() {
  const uint8_t bases[] = {0x00, 0x20, 0x40, 0x60, 0xc0, 0xe0};
  const uint8_t offsets[] = {0x01, 0x03, 0x05, 0x07, 0x0d, 0x0f, 0x11,
                             0x12, 0x13, 0x15, 0x17, 0x19, 0x1d, 0x1f};
  for (unsigned group = 0; group < sizeof(bases); ++group) {
    for (unsigned mode = 0; mode < sizeof(offsets); ++mode) {
      SgbHost host;
      uint8_t code[4] = {uint8_t(bases[group] + offsets[mode]), 0, 0, 0};
      size_t length = 0;
      const uint16_t address = prepareMemoryOperand(host, mode, code, length);
      uint8_t left = 0;
      uint8_t right = 0;
      uint8_t expected = 0;
      switch (group) {
      case 0: left = 0xf0; right = 0x0f; expected = 0xff; break; // ORA
      case 1: left = 0xf0; right = 0x0f; expected = 0x00; break; // AND
      case 2: left = 0xaa; right = 0xff; expected = 0x55; break; // EOR
      case 3: left = 1; right = 2; expected = 3; break; // ADC
      case 4: left = 5; right = 5; expected = 5; break; // CMP
      default: left = 5; right = 2; expected = 3; break; // SBC
      }
      host.cpu.state().a = 0x5a00 | left;
      host.cpu.state().p = (host.cpu.state().p & ~(C | 0x08)) |
          (group == 5 ? C : 0);
      host.wram[address] = right;
      instruction(host, code, length);
      assert((host.cpu.state().a & 0xff) == expected);
      assert((host.cpu.state().a >> 8) == 0x5a);
      if (group == 1)
        assert(host.cpu.state().p & Z);
      if (group == 4)
        assert((host.cpu.state().p & (C | Z)) == (C | Z));
      if (group == 5)
        assert(host.cpu.state().p & C);
    }
  }

  SgbHost wide;
  native16(wide);
  wide.cpu.state().a = 0x0ff0;
  wide.cpu.state().dbr = 0x7e;
  wide.wram[0x02ff] = 0xff;
  wide.wram[0x0300] = 0x00;
  const uint8_t and16[] = {0x2d, 0xff, 0x02};
  instruction(wide, and16, sizeof(and16));
  assert(wide.cpu.state().a == 0x00f0);
}

static void testControlPointerAndPushFamilies() {
  {
    SgbHost host;
    const uint8_t wdm[] = {0x42, 0xa5};
    instruction(host, wdm, sizeof(wdm));
    assert(host.cpu.state().pc == 2);
  }
  {
    SgbHost host;
    const uint8_t brl[] = {0x82, 0xfc, 0xff};
    instruction(host, brl, sizeof(brl));
    assert(host.cpu.state().pc == 0xffff);
  }
  {
    SgbHost host;
    host.wram[0x0200] = 0x34;
    host.wram[0x0201] = 0x12;
    const uint8_t jmpIndirect[] = {0x6c, 0x00, 0x02};
    instruction(host, jmpIndirect, sizeof(jmpIndirect));
    assert(host.cpu.state().pc == 0x1234 && host.cpu.state().pbr == 0x7e);
  }
  {
    SgbHost host;
    host.cpu.state().x = 2;
    host.wram[0x0202] = 0x78;
    host.wram[0x0203] = 0x56;
    const uint8_t jmpIndexed[] = {0x7c, 0x00, 0x02};
    instruction(host, jmpIndexed, sizeof(jmpIndexed));
    assert(host.cpu.state().pc == 0x5678 && host.cpu.state().pbr == 0x7e);
  }
  {
    SgbHost host;
    host.wram[0x0200] = 0x34;
    host.wram[0x0201] = 0x12;
    host.wram[0x0202] = 0x7f;
    const uint8_t jmpLongIndirect[] = {0xdc, 0x00, 0x02};
    instruction(host, jmpLongIndirect, sizeof(jmpLongIndirect));
    assert(host.cpu.state().pc == 0x1234 && host.cpu.state().pbr == 0x7f);
  }
  {
    SgbHost host;
    host.cpu.state().x = 2;
    host.wram[0x0202] = 0xbc;
    host.wram[0x0203] = 0x9a;
    const uint8_t jsrIndexed[] = {0xfc, 0x00, 0x02};
    instruction(host, jsrIndexed, sizeof(jsrIndexed));
    assert(host.cpu.state().pc == 0x9abc);
    assert(host.cpu.state().sp == 0x01fd);
    assert(host.wram[0x01ff] == 0 && host.wram[0x01fe] == 2);
  }
  {
    SgbHost host;
    const uint8_t per[] = {0x62, 0x05, 0x00};
    instruction(host, per, sizeof(per));
    assert(host.cpu.state().sp == 0x01fd);
    assert(host.wram[0x01ff] == 0 && host.wram[0x01fe] == 8);
  }
  {
    SgbHost host;
    const uint8_t pea[] = {0xf4, 0x34, 0x12};
    instruction(host, pea, sizeof(pea));
    assert(host.wram[0x01ff] == 0x12 && host.wram[0x01fe] == 0x34);
  }
  {
    SgbHost host;
    host.cpu.state().d = 0x0100;
    host.wram[0x0120] = 0x78;
    host.wram[0x0121] = 0x56;
    const uint8_t pei[] = {0xd4, 0x20};
    instruction(host, pei, sizeof(pei));
    assert(host.wram[0x01ff] == 0x56 && host.wram[0x01fe] == 0x78);
  }

  struct CompareMemory { uint8_t op; bool y; bool absolute; } compares[] = {
      {0xc4, true, false}, {0xcc, true, true},
      {0xe4, false, false}, {0xec, false, true}};
  for (unsigned i = 0; i < sizeof(compares) / sizeof(compares[0]); ++i) {
    SgbHost host;
    host.cpu.state().d = 0x0100;
    host.cpu.state().dbr = 0x7e;
    if (compares[i].y) host.cpu.state().y = 5;
    else host.cpu.state().x = 5;
    host.wram[compares[i].absolute ? 0x0220 : 0x0120] = 5;
    const uint8_t code[] = {compares[i].op, 0x20, 0x02};
    instruction(host, code, compares[i].absolute ? 3 : 2);
    assert((host.cpu.state().p & (C | Z | N)) == (C | Z));
  }
}

static void testBlockMoves() {
  SgbHost forward;
  native16(forward);
  forward.cpu.state().a = 1;
  forward.cpu.state().x = 0x0100;
  forward.cpu.state().y = 0x0200;
  forward.wram[0x0100] = 0x11;
  forward.wram[0x0101] = 0x22;
  const uint8_t mvn[] = {0x54, 0x7f, 0x7e};
  memset(forward.wram, 0, 16);
  memcpy(forward.wram, mvn, sizeof(mvn));
  forward.cpu.jump(0x7e0000, 0);
  assert(forward.cpu.run(forward, 2) == 2);
  assert(!forward.cpu.state().faulted);
  assert(forward.wram[0x10200] == 0x11);
  assert(forward.wram[0x10201] == 0x22);
  assert(forward.cpu.state().a == 0xffff);
  assert(forward.cpu.state().x == 0x0102);
  assert(forward.cpu.state().y == 0x0202);
  assert(forward.cpu.state().dbr == 0x7f);
  assert(forward.cpu.state().pc == 3);

  SgbHost backward;
  native16(backward);
  backward.cpu.state().a = 0;
  backward.cpu.state().x = 0x0100;
  backward.cpu.state().y = 0x0200;
  backward.wram[0x10100] = 0xa5;
  const uint8_t mvp[] = {0x44, 0x7e, 0x7f};
  instruction(backward, mvp, sizeof(mvp));
  assert(backward.wram[0x0200] == 0xa5);
  assert(backward.cpu.state().a == 0xffff);
  assert(backward.cpu.state().x == 0x00ff);
  assert(backward.cpu.state().y == 0x01ff);
  assert(backward.cpu.state().dbr == 0x7e);
}

static void testSoftwareInterrupts() {
  const uint8_t opcodes[] = {0x00, 0x02};
  for (unsigned i = 0; i < sizeof(opcodes); ++i) {
    SgbHost emulation;
    emulation.cpu.state().p |= 0x08;
    const uint8_t code[] = {opcodes[i], 0xa5};
    instruction(emulation, code, sizeof(code));
    assert(emulation.cpu.state().pc == 0xffff);
    assert(emulation.cpu.state().pbr == 0);
    assert(emulation.cpu.state().sp == 0x01fc);
    assert(emulation.wram[0x01ff] == 0);
    assert(emulation.wram[0x01fe] == 2);
    assert(emulation.wram[0x01fd] & X);
    assert(emulation.cpu.state().p & 0x04);
    assert(!(emulation.cpu.state().p & 0x08));

    SgbHost native;
    native16(native);
    native.cpu.state().p |= 0x08;
    instruction(native, code, sizeof(code));
    assert(native.cpu.state().pc == 0xffff);
    assert(native.cpu.state().pbr == 0);
    assert(native.cpu.state().sp == 0x01fb);
    assert(native.wram[0x01ff] == 0x7e);
    assert(native.wram[0x01fe] == 0);
    assert(native.wram[0x01fd] == 2);
    assert(!(native.wram[0x01fc] & (M | X)));
    assert(native.cpu.state().p & 0x04);
    assert(!(native.cpu.state().p & 0x08));
  }
}

int main() {
  testImmediateLogicAndCompare();
  testAccumulatorShifts();
  testWidthsTransfersAndStack();
  testLoadStoreAddressing();
  testImmediateArithmetic();
  testMemoryShiftAndUpdateFamilies();
  testMemoryBitFamilies();
  testIndirectLoadStoreAddressing();
  testMemoryArithmeticAddressing();
  testControlPointerAndPushFamilies();
  testBlockMoves();
  testSoftwareInterrupts();
  puts("65C816 instruction vectors passed");
}
