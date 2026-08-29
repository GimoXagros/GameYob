#include "sgb_host.h"
#include <string.h>

// Keep the portable host core independent of platform filesystem headers.
// These declarations match io.h and are used only by the versioned state I/O.
void file_read(void *, int, int, FileHandle *);
void file_write(const void *, int, int, FileHandle *);

namespace {
enum {
  FLAG_C = 0x01,
  FLAG_Z = 0x02,
  FLAG_I = 0x04,
  FLAG_D = 0x08,
  FLAG_X = 0x10,
  FLAG_M = 0x20,
  FLAG_V = 0x40,
  FLAG_N = 0x80
};

static uint16_t read16(SgbHost &host, uint32_t address) {
  return host.read8(address) | (host.read8((address + 1) & 0xffffff) << 8);
}

static void write16(SgbHost &host, uint32_t address, uint16_t value) {
  host.write8(address, value & 0xff);
  host.write8((address + 1) & 0xffffff, value >> 8);
}

static int16_t clamp16(int value) {
  if (value < -32768)
    return -32768;
  if (value > 32767)
    return 32767;
  return (int16_t)value;
}
} // namespace

SgbHostCpu::SgbHostCpu() { reset(); }

void SgbHostCpu::reset() {
  memset(&cpu, 0, sizeof(cpu));
  cpu.sp = 0x1ff;
  cpu.p = FLAG_I | FLAG_M | FLAG_X;
  cpu.emulation = 1;
  nmiPending = false;
}

void SgbHostCpu::jump(uint32_t address, uint32_t nmiAddress) {
  cpu.pc = address & 0xffff;
  cpu.pbr = (address >> 16) & 0xff;
  cpu.nmiVector = nmiAddress & 0xffffff;
  cpu.waiting = cpu.stopped = cpu.faulted = 0;
}

void SgbHostCpu::requestNmi() { nmiPending = true; }

int SgbHostCpu::run(SgbHost &host, int budget) {
  int executed = 0;
  bool m8 = true;
  bool x8 = true;
  uint16_t xMask = 0xff;
  auto fetch = [&]() -> uint8_t {
    uint8_t value = host.read8((cpu.pbr << 16) | cpu.pc);
    cpu.pc++;
    return value;
  };
  auto push = [&](uint8_t value) {
    host.write8(cpu.sp, value);
    cpu.sp = (cpu.sp - 1) & (cpu.emulation ? 0x1ff : 0xffff);
    if (cpu.emulation)
      cpu.sp |= 0x100;
  };
  auto pop = [&]() -> uint8_t {
    cpu.sp = (cpu.sp + 1) & (cpu.emulation ? 0x1ff : 0xffff);
    if (cpu.emulation)
      cpu.sp |= 0x100;
    return host.read8(cpu.sp);
  };
  auto setNz8 = [&](uint8_t value) {
    cpu.p = (cpu.p & ~(FLAG_N | FLAG_Z)) | (value == 0 ? FLAG_Z : 0) |
            (value & FLAG_N);
  };
  auto setNz16 = [&](uint16_t value) {
    cpu.p = (cpu.p & ~(FLAG_N | FLAG_Z)) | (value == 0 ? FLAG_Z : 0) |
            ((value >> 8) & FLAG_N);
  };
  auto branch = [&](bool condition) {
    int8_t offset = (int8_t)fetch();
    if (condition)
      cpu.pc = uint16_t(cpu.pc + offset);
  };

  while (executed < budget && !cpu.stopped && !cpu.faulted) {
    if (nmiPending && cpu.nmiVector) {
      nmiPending = false;
      cpu.waiting = 0;
      push(cpu.pbr);
      push(cpu.pc >> 8);
      push(cpu.pc & 0xff);
      push(cpu.p);
      cpu.p |= FLAG_I;
      cpu.pbr = (cpu.nmiVector >> 16) & 0xff;
      cpu.pc = cpu.nmiVector & 0xffff;
    }
    if (cpu.waiting)
      break;
    m8 = (cpu.p & FLAG_M) != 0;
    x8 = (cpu.p & FLAG_X) != 0;
    xMask = x8 ? 0xff : 0xffff;
    uint8_t op = fetch();
    executed++;
    switch (op) {
    case 0xea:
      break; // NOP
    case 0x18:
      cpu.p &= ~FLAG_C;
      break;
    case 0x38:
      cpu.p |= FLAG_C;
      break;
    case 0x58:
      cpu.p &= ~FLAG_I;
      break;
    case 0x78:
      cpu.p |= FLAG_I;
      break;
    case 0xb8:
      cpu.p &= ~FLAG_V;
      break;
    case 0xd8:
      cpu.p &= ~FLAG_D;
      break;
    case 0xf8:
      cpu.p |= FLAG_D;
      break;
    case 0xc2:
      cpu.p &= ~fetch();
      break;
    case 0xe2:
      cpu.p |= fetch();
      break;
    case 0xfb: {
      bool oldCarry = (cpu.p & FLAG_C) != 0;
      if (cpu.emulation)
        cpu.p |= FLAG_C;
      else
        cpu.p &= ~FLAG_C;
      cpu.emulation = oldCarry;
      if (cpu.emulation) {
        cpu.p |= FLAG_M | FLAG_X;
        cpu.sp = 0x100 | (cpu.sp & 0xff);
      }
      break;
    }
    case 0xa9:
      if (m8) {
        cpu.a = (cpu.a & 0xff00) | fetch();
        setNz8(cpu.a);
      } else {
        cpu.a = fetch();
        cpu.a |= fetch() << 8;
        setNz16(cpu.a);
      }
      break;
    case 0xa2:
      cpu.x = fetch();
      if (!x8)
        cpu.x |= fetch() << 8;
      if (x8)
        setNz8(cpu.x);
      else
        setNz16(cpu.x);
      break;
    case 0xa0:
      cpu.y = fetch();
      if (!x8)
        cpu.y |= fetch() << 8;
      if (x8)
        setNz8(cpu.y);
      else
        setNz16(cpu.y);
      break;
    case 0x8d: {
      uint32_t a = (cpu.dbr << 16) | fetch();
      a |= uint32_t(fetch()) << 8;
      if (m8)
        host.write8(a, cpu.a);
      else
        write16(host, a, cpu.a);
      break;
    }
    case 0x8f: {
      uint32_t a = fetch();
      a |= uint32_t(fetch()) << 8;
      a |= uint32_t(fetch()) << 16;
      if (m8)
        host.write8(a, cpu.a);
      else
        write16(host, a, cpu.a);
      break;
    }
    case 0x8e: {
      uint32_t a = (cpu.dbr << 16) | fetch();
      a |= uint32_t(fetch()) << 8;
      if (x8)
        host.write8(a, cpu.x);
      else
        write16(host, a, cpu.x);
      break;
    }
    case 0x8c: {
      uint32_t a = (cpu.dbr << 16) | fetch();
      a |= uint32_t(fetch()) << 8;
      if (x8)
        host.write8(a, cpu.y);
      else
        write16(host, a, cpu.y);
      break;
    }
    case 0x9c: {
      uint32_t a = (cpu.dbr << 16) | fetch();
      a |= uint32_t(fetch()) << 8;
      if (m8)
        host.write8(a, 0);
      else
        write16(host, a, 0);
      break;
    }
    case 0xad: {
      uint32_t a = (cpu.dbr << 16) | fetch();
      a |= uint32_t(fetch()) << 8;
      cpu.a = m8 ? host.read8(a) : read16(host, a);
      if (m8)
        setNz8(cpu.a);
      else
        setNz16(cpu.a);
      break;
    }
    case 0xaf: {
      uint32_t a = fetch();
      a |= uint32_t(fetch()) << 8;
      a |= uint32_t(fetch()) << 16;
      cpu.a = m8 ? host.read8(a) : read16(host, a);
      if (m8)
        setNz8(cpu.a);
      else
        setNz16(cpu.a);
      break;
    }
    case 0x4c:
      cpu.pc = fetch();
      cpu.pc |= fetch() << 8;
      break;
    case 0x5c: {
      uint32_t a = fetch();
      a |= uint32_t(fetch()) << 8;
      a |= uint32_t(fetch()) << 16;
      cpu.pc = a;
      cpu.pbr = a >> 16;
      break;
    }
    case 0x20: {
      uint16_t a = fetch();
      a |= fetch() << 8;
      uint16_t ret = cpu.pc - 1;
      push(ret >> 8);
      push(ret);
      cpu.pc = a;
      break;
    }
    case 0x22: {
      uint32_t a = fetch();
      a |= uint32_t(fetch()) << 8;
      a |= uint32_t(fetch()) << 16;
      uint16_t ret = cpu.pc - 1;
      push(cpu.pbr);
      push(ret >> 8);
      push(ret);
      cpu.pc = a;
      cpu.pbr = a >> 16;
      break;
    }
    case 0x60: {
      uint16_t a = pop();
      a |= pop() << 8;
      cpu.pc = a + 1;
      break;
    }
    case 0x6b: {
      uint16_t a = pop();
      a |= pop() << 8;
      cpu.pbr = pop();
      cpu.pc = a + 1;
      break;
    }
    case 0x40:
      cpu.p = pop();
      cpu.pc = pop();
      cpu.pc |= pop() << 8;
      if (!cpu.emulation)
        cpu.pbr = pop();
      break;
    case 0x80:
      branch(true);
      break;
    case 0x10:
      branch(!(cpu.p & FLAG_N));
      break;
    case 0x30:
      branch(cpu.p & FLAG_N);
      break;
    case 0x50:
      branch(!(cpu.p & FLAG_V));
      break;
    case 0x70:
      branch(cpu.p & FLAG_V);
      break;
    case 0x90:
      branch(!(cpu.p & FLAG_C));
      break;
    case 0xb0:
      branch(cpu.p & FLAG_C);
      break;
    case 0xd0:
      branch(!(cpu.p & FLAG_Z));
      break;
    case 0xf0:
      branch(cpu.p & FLAG_Z);
      break;
    case 0xaa:
      cpu.x = cpu.a & xMask;
      if (x8)
        setNz8(cpu.x);
      else
        setNz16(cpu.x);
      break;
    case 0xa8:
      cpu.y = cpu.a & xMask;
      if (x8)
        setNz8(cpu.y);
      else
        setNz16(cpu.y);
      break;
    case 0x8a:
      cpu.a = (cpu.a & 0xff00) | (cpu.x & 0xff);
      setNz8(cpu.a);
      break;
    case 0x98:
      cpu.a = (cpu.a & 0xff00) | (cpu.y & 0xff);
      setNz8(cpu.a);
      break;
    case 0xe8:
      cpu.x = (cpu.x + 1) & xMask;
      if (x8)
        setNz8(cpu.x);
      else
        setNz16(cpu.x);
      break;
    case 0xc8:
      cpu.y = (cpu.y + 1) & xMask;
      if (x8)
        setNz8(cpu.y);
      else
        setNz16(cpu.y);
      break;
    case 0xca:
      cpu.x = (cpu.x - 1) & xMask;
      if (x8)
        setNz8(cpu.x);
      else
        setNz16(cpu.x);
      break;
    case 0x88:
      cpu.y = (cpu.y - 1) & xMask;
      if (x8)
        setNz8(cpu.y);
      else
        setNz16(cpu.y);
      break;
    case 0x48:
      if (!m8)
        push(cpu.a >> 8);
      push(cpu.a);
      break;
    case 0x68:
      cpu.a = pop();
      if (!m8)
        cpu.a |= pop() << 8;
      if (m8)
        setNz8(cpu.a);
      else
        setNz16(cpu.a);
      break;
    case 0xda:
      if (!x8)
        push(cpu.x >> 8);
      push(cpu.x);
      break;
    case 0xfa:
      cpu.x = pop();
      if (!x8)
        cpu.x |= pop() << 8;
      if (x8)
        setNz8(cpu.x);
      else
        setNz16(cpu.x);
      break;
    case 0xcb:
      cpu.waiting = 1;
      break;
    case 0xdb:
      cpu.stopped = 1;
      break;
    default:
      cpu.faulted = 1;
      cpu.faultOpcode = op;
      break;
    }
    if (cpu.emulation)
      cpu.p |= FLAG_M | FLAG_X;
  }
  return executed;
}

SgbHostPpu::SgbHostPpu() { reset(); }
void SgbHostPpu::reset() {
  memset(vram, 0, sizeof(vram));
  memset(cgram, 0, sizeof(cgram));
  memset(oam, 0, sizeof(oam));
  vramAddress = cgramAddress = oamAddress = 0;
  objectControl = objectEnabled = 0;
}

uint8_t SgbHostPpu::readRegister(uint16_t address) {
  if (address == 0x2139)
    return vram[vramAddress++];
  if (address == 0x213b)
    return cgram[(cgramAddress++) & 0x1ff];
  if (address == 0x2138)
    return oam[(oamAddress++) % sizeof(oam)];
  return 0;
}

void SgbHostPpu::writeRegister(uint16_t address, uint8_t value) {
  switch (address) {
  case 0x2101:
    objectControl = value;
    break;
  case 0x2102:
    oamAddress = (oamAddress & 0x100) | value;
    break;
  case 0x2103:
    oamAddress = (oamAddress & 0xff) | ((value & 1) << 8);
    break;
  case 0x2104:
    oam[(oamAddress++) % sizeof(oam)] = value;
    break;
  case 0x2116:
    vramAddress = (vramAddress & 0xff00) | value;
    break;
  case 0x2117:
    vramAddress = (vramAddress & 0xff) | (value << 8);
    break;
  case 0x2118:
    vram[vramAddress++] = value;
    break;
  case 0x2119:
    vram[vramAddress++] = value;
    break;
  case 0x2121:
    cgramAddress = value * 2;
    break;
  case 0x2122:
    cgram[(cgramAddress++) & 0x1ff] = value;
    break;
  }
}

void SgbHostPpu::loadCharacterData(const uint8_t *data, uint8_t destination) {
  if (data)
    memcpy(vram + ((destination & 1) ? 0x1000 : 0), data, 0x1000);
}

void SgbHostPpu::loadPrototypeObjects(const uint8_t *data, size_t length,
                                      const uint8_t *palettes,
                                      const uint16_t ids[4]) {
  if (!data || length < 0x66)
    return;
  memcpy(oam, data, 0x66);
  if (palettes && ids) {
    for (int p = 0; p < 4; ++p) {
      size_t source = (ids[p] & 0x1ff) * 8;
      for (int c = 0; c < 4; ++c) {
        cgram[(4 + p) * 32 + c * 2] = palettes[source + c * 2];
        cgram[(4 + p) * 32 + c * 2 + 1] = palettes[source + c * 2 + 1];
      }
    }
  }
  objectEnabled = 1;
}

SgbHostPpu::ObjectPixel SgbHostPpu::objectPixel(int x, int y) const {
  ObjectPixel result = {0, 0, 0};
  if (!objectEnabled)
    return result;
  for (int i = 23; i >= 0; --i) {
    const int ox = oam[i * 4] | ((oam[0x60 + i / 4] >> ((i & 3) * 2)) & 1) << 8;
    const int oy = oam[i * 4 + 1];
    const uint8_t tile = oam[i * 4 + 2];
    const uint8_t attr = oam[i * 4 + 3];
    int px = x - ox, py = y - oy;
    if (px < 0 || px >= 8 || py < 0 || py >= 8)
      continue;
    if (attr & 0x40)
      px = 7 - px;
    if (attr & 0x80)
      py = 7 - py;
    const uint32_t base = tile * 32 + py * 2;
    const int bit = 7 - px;
    int color = ((vram[(base + 0) & 0xffff] >> bit) & 1) |
                (((vram[(base + 1) & 0xffff] >> bit) & 1) << 1) |
                (((vram[(base + 16) & 0xffff] >> bit) & 1) << 2) |
                (((vram[(base + 17) & 0xffff] >> bit) & 1) << 3);
    if (!color)
      continue;
    int palette = (attr >> 1) & 7;
    size_t ci = (palette * 16 + color) * 2;
    result.color = cgram[ci] | (cgram[ci + 1] << 8);
    result.priority = (attr >> 4) & 3;
    result.visible = 1;
    return result;
  }
  return result;
}

SgbHostApu::SgbHostApu() { reset(); }
void SgbHostApu::reset() {
  memset(ram, 0, sizeof(ram));
  memset(dsp, 0, sizeof(dsp));
  memset(inputPorts, 0, sizeof(inputPorts));
  memset(&cpu, 0, sizeof(cpu));
  memset(voices, 0, sizeof(voices));
  dspAddress = 0;
  cpu.sp = 0xff;
}
uint8_t SgbHostApu::read(uint16_t a) const {
  return a >= 0xf4 && a <= 0xf7 ? inputPorts[a - 0xf4] : ram[a];
}
void SgbHostApu::write(uint16_t a, uint8_t v) {
  if (a == 0xf2)
    dspAddress = v & 0x7f;
  else if (a == 0xf3) {
    dsp[dspAddress] = v;
    if (dspAddress == 0x4c)
      keyOn(v);
  } else
    ram[a] = v;
}

bool SgbHostApu::transferProgram(const uint8_t *data, size_t length) {
  if (!data)
    return false;
  size_t pos = 0;
  bool any = false;
  while (pos + 4 <= length) {
    uint16_t count = data[pos] | (data[pos + 1] << 8),
             dest = data[pos + 2] | (data[pos + 3] << 8);
    pos += 4;
    if (!count) {
      cpu.pc = dest;
      cpu.stopped = cpu.faulted = 0;
      return true;
    }
    if (pos + count > length)
      return false;
    size_t copy = count;
    const size_t available = 0x10000u - dest;
    if (copy > available)
      copy = available;
    memcpy(ram + dest, data + pos, copy);
    pos += count;
    any = true;
  }
  return any;
}

void SgbHostApu::applySoundCommand(const uint8_t *command, size_t length) {
  for (size_t i = 0; i < length && i < 4; ++i)
    inputPorts[i] = command[i];
}

int SgbHostApu::run(int budget) {
  int done = 0;
  auto fetch = [&]() { return read(cpu.pc++); };
  auto nz = [&](uint8_t v) {
    cpu.psw = (cpu.psw & ~0x82) | (v ? 0 : 2) | (v & 0x80);
  };
  while (done < budget && !cpu.stopped && !cpu.faulted) {
    uint8_t op = fetch();
    done++;
    switch (op) {
    case 0x00:
      break;
    case 0xe8:
      cpu.a = fetch();
      nz(cpu.a);
      break;
    case 0xcd:
      cpu.x = fetch();
      nz(cpu.x);
      break;
    case 0x8d:
      cpu.y = fetch();
      nz(cpu.y);
      break;
    case 0xc4:
      write(fetch(), cpu.a);
      break;
    case 0xc5: {
      uint16_t a = fetch();
      a |= fetch() << 8;
      write(a, cpu.a);
      break;
    }
    case 0xe4:
      cpu.a = read(fetch());
      nz(cpu.a);
      break;
    case 0xe5: {
      uint16_t a = fetch();
      a |= fetch() << 8;
      cpu.a = read(a);
      nz(cpu.a);
      break;
    }
    case 0xbc:
      cpu.a++;
      nz(cpu.a);
      break;
    case 0x9c:
      cpu.a--;
      nz(cpu.a);
      break;
    case 0x2f:
      cpu.pc = uint16_t(cpu.pc + (int8_t)fetch());
      break;
    case 0xd0: {
      int8_t o = (int8_t)fetch();
      if (!(cpu.psw & 2))
        cpu.pc = uint16_t(cpu.pc + o);
      break;
    }
    case 0xf0: {
      int8_t o = (int8_t)fetch();
      if (cpu.psw & 2)
        cpu.pc = uint16_t(cpu.pc + o);
      break;
    }
    case 0x5f: {
      uint16_t a = fetch();
      a |= fetch() << 8;
      cpu.pc = a;
      break;
    }
    case 0xff:
    case 0xef:
      cpu.stopped = 1;
      break;
    case 0x5d:
      cpu.x = cpu.a;
      nz(cpu.x);
      break;
    case 0x7d:
      cpu.a = cpu.x;
      nz(cpu.a);
      break;
    case 0xfd:
      cpu.y = cpu.a;
      nz(cpu.y);
      break;
    case 0xdd:
      cpu.a = cpu.y;
      nz(cpu.a);
      break;
    case 0xbd:
      cpu.sp = cpu.x;
      break;
    default:
      cpu.faulted = 1;
      cpu.faultOpcode = op;
      break;
    }
  }
  return done;
}

void SgbHostApu::keyOn(uint8_t mask) {
  uint16_t dir = dsp[0x5d] << 8;
  for (int i = 0; i < 8; ++i)
    if (mask & (1 << i)) {
      uint8_t src = dsp[i * 16 + 4];
      uint16_t ent = dir + src * 4;
      voices[i].brrAddress = ram[ent] | (ram[(uint16_t)(ent + 1)] << 8);
      voices[i].nibble = 0;
      voices[i].phase = 0;
      voices[i].previous1 = voices[i].previous2 = voices[i].sample = 0;
      voices[i].active = 1;
    }
}

int16_t SgbHostApu::decodeBrrSample(int v) {
  VoiceState &s = voices[v];
  if (!s.active)
    return 0;
  uint8_t header = ram[s.brrAddress],
          packed = ram[(uint16_t)(s.brrAddress + 1 + s.nibble / 2)];
  int sample = (s.nibble & 1) ? (packed & 15) : (packed >> 4);
  if (sample & 8)
    sample -= 16;
  int shift = header >> 4;
  sample = shift <= 12 ? (sample << shift) >> 1 : (sample < 0 ? -2048 : 0);
  switch ((header >> 2) & 3) {
  case 1:
    sample += s.previous1 * 15 / 16;
    break;
  case 2:
    sample += s.previous1 * 61 / 32 - s.previous2 * 15 / 16;
    break;
  case 3:
    sample += s.previous1 * 115 / 64 - s.previous2 * 13 / 16;
    break;
  }
  sample = clamp16(sample);
  s.previous2 = s.previous1;
  s.previous1 = sample;
  s.nibble++;
  if (s.nibble == 16) {
    s.nibble = 0;
    if (header & 1) {
      if (header & 2) {
        uint16_t dir = dsp[0x5d] << 8;
        uint8_t src = dsp[v * 16 + 4];
        uint16_t e = dir + src * 4 + 2;
        s.brrAddress = ram[e] | (ram[(uint16_t)(e + 1)] << 8);
      } else
        s.active = 0;
    } else
      s.brrAddress += 9;
  }
  return sample;
}

void SgbHostApu::render(int16_t *output, size_t count) {
  if (!output)
    return;
  for (size_t n = 0; n < count; ++n) {
    int mix = 0;
    for (int v = 0; v < 8; ++v) {
      VoiceState &s = voices[v];
      uint16_t pitch = dsp[v * 16 + 2] | ((dsp[v * 16 + 3] & 0x3f) << 8);
      s.phase += pitch;
      while (s.phase >= 0x1000) {
        s.phase -= 0x1000;
        s.sample = decodeBrrSample(v);
      }
      int vol = ((int8_t)dsp[v * 16] + (int8_t)dsp[v * 16 + 1]) / 2;
      mix += s.sample * vol / 128;
    }
    output[n] = clamp16(mix);
  }
}

bool SgbHostApu::hasActiveAudio() const {
  for (int i = 0; i < 8; ++i)
    if (voices[i].active)
      return true;
  return false;
}

SgbHost::SgbHost() { reset(); }
void SgbHost::reset() {
  memset(wram, 0, sizeof(wram));
  cpu.reset();
  ppu.reset();
  apu.reset();
}

uint8_t SgbHost::read8(uint32_t a) {
  a &= 0xffffff;
  uint8_t bank = a >> 16;
  uint16_t off = a;
  if (bank == 0x7e || bank == 0x7f)
    return wram[((bank - 0x7e) << 16) | off];
  if (((bank <= 0x3f) || (bank >= 0x80 && bank <= 0xbf)) && off < 0x2000)
    return wram[off];
  if (((bank <= 0x3f) || (bank >= 0x80 && bank <= 0xbf)) && off >= 0x2100 &&
      off <= 0x213f)
    return ppu.readRegister(off);
  if (((bank <= 0x3f) || (bank >= 0x80 && bank <= 0xbf)) && off >= 0x2140 &&
      off <= 0x2143)
    return apu.inputPorts[off - 0x2140];
  return 0xff;
}

void SgbHost::write8(uint32_t a, uint8_t v) {
  a &= 0xffffff;
  uint8_t bank = a >> 16;
  uint16_t off = a;
  if (bank == 0x7e || bank == 0x7f) {
    wram[((bank - 0x7e) << 16) | off] = v;
    return;
  }
  if (((bank <= 0x3f) || (bank >= 0x80 && bank <= 0xbf)) && off < 0x2000) {
    wram[off] = v;
    return;
  }
  if (((bank <= 0x3f) || (bank >= 0x80 && bank <= 0xbf)) && off >= 0x2100 &&
      off <= 0x213f) {
    ppu.writeRegister(off, v);
    return;
  }
  if (((bank <= 0x3f) || (bank >= 0x80 && bank <= 0xbf)) && off >= 0x2140 &&
      off <= 0x2143) {
    apu.inputPorts[off - 0x2140] = v;
    return;
  }
}

bool SgbHost::writeMemory(uint32_t address, const uint8_t *data,
                          size_t length) {
  if (!data)
    return false;
  for (size_t i = 0; i < length; ++i) {
    uint32_t a = (address + i) & 0xffffff;
    uint8_t bank = a >> 16;
    uint16_t off = a;
    bool valid = bank == 0x7e || bank == 0x7f ||
                 (((bank <= 0x3f) || (bank >= 0x80 && bank <= 0xbf)) &&
                  (off < 0x2000 || (off >= 0x2100 && off <= 0x2143)));
    if (!valid)
      return false;
  }
  for (size_t i = 0; i < length; ++i)
    write8((address + i) & 0xffffff, data[i]);
  return true;
}

void SgbHost::jump(uint32_t a, uint32_t n) {
  cpu.jump(a, n);
  cpu.run(*this, 4096);
}
void SgbHost::runFrame() {
  cpu.requestNmi();
  cpu.run(*this, 8192);
  apu.run(4096);
}
void SgbHost::renderAudio(int16_t *o, size_t n) { apu.render(o, n); }

bool SgbHost::saveState(FileHandle *f) const {
  if (!f)
    return false;
  const uint32_t magic = 0x53474248, version = STATE_FORMAT;
  const SgbHostCpu::State &c = cpu.state();
  file_write(&magic, 4, 1, f);
  file_write(&version, 4, 1, f);
  file_write(&c.a, 1, sizeof(c.a), f);
  file_write(&c.x, 1, sizeof(c.x), f);
  file_write(&c.y, 1, sizeof(c.y), f);
  file_write(&c.sp, 1, sizeof(c.sp), f);
  file_write(&c.d, 1, sizeof(c.d), f);
  file_write(&c.pc, 1, sizeof(c.pc), f);
  file_write(&c.pbr, 1, 1, f);
  file_write(&c.dbr, 1, 1, f);
  file_write(&c.p, 1, 1, f);
  file_write(&c.emulation, 1, 1, f);
  file_write(&c.waiting, 1, 1, f);
  file_write(&c.stopped, 1, 1, f);
  file_write(&c.faulted, 1, 1, f);
  file_write(&c.faultOpcode, 1, 1, f);
  file_write(&c.nmiVector, 1, sizeof(c.nmiVector), f);
  file_write(wram, 1, sizeof(wram), f);
  file_write(ppu.vram, 1, sizeof(ppu.vram), f);
  file_write(ppu.cgram, 1, sizeof(ppu.cgram), f);
  file_write(ppu.oam, 1, sizeof(ppu.oam), f);
  file_write(&ppu.vramAddress, 1, sizeof(ppu.vramAddress), f);
  file_write(&ppu.cgramAddress, 1, sizeof(ppu.cgramAddress), f);
  file_write(&ppu.oamAddress, 1, sizeof(ppu.oamAddress), f);
  file_write(&ppu.objectControl, 1, 1, f);
  file_write(&ppu.objectEnabled, 1, 1, f);
  file_write(apu.ram, 1, sizeof(apu.ram), f);
  file_write(apu.dsp, 1, sizeof(apu.dsp), f);
  file_write(&apu.dspAddress, 1, 1, f);
  file_write(apu.inputPorts, 1, sizeof(apu.inputPorts), f);
  file_write(&apu.cpu.pc, 1, sizeof(apu.cpu.pc), f);
  file_write(&apu.cpu.sp, 1, sizeof(apu.cpu.sp), f);
  file_write(&apu.cpu.a, 1, 1, f);
  file_write(&apu.cpu.x, 1, 1, f);
  file_write(&apu.cpu.y, 1, 1, f);
  file_write(&apu.cpu.psw, 1, 1, f);
  file_write(&apu.cpu.stopped, 1, 1, f);
  file_write(&apu.cpu.faulted, 1, 1, f);
  file_write(&apu.cpu.faultOpcode, 1, 1, f);
  for (int i = 0; i < 8; ++i) {
    const SgbHostApu::VoiceState &v = apu.voices[i];
    file_write(&v.phase, 1, sizeof(v.phase), f);
    file_write(&v.brrAddress, 1, sizeof(v.brrAddress), f);
    file_write(&v.nibble, 1, 1, f);
    file_write(&v.previous1, 1, sizeof(v.previous1), f);
    file_write(&v.previous2, 1, sizeof(v.previous2), f);
    file_write(&v.sample, 1, sizeof(v.sample), f);
    file_write(&v.active, 1, 1, f);
  }
  return true;
}

bool SgbHost::loadState(FileHandle *f) {
  if (!f)
    return false;
  uint32_t magic = 0, version = 0;
  file_read(&magic, 4, 1, f);
  file_read(&version, 4, 1, f);
  if (magic != 0x53474248 || version != STATE_FORMAT)
    return false;
  SgbHostCpu::State &c = cpu.state();
  file_read(&c.a, 1, sizeof(c.a), f);
  file_read(&c.x, 1, sizeof(c.x), f);
  file_read(&c.y, 1, sizeof(c.y), f);
  file_read(&c.sp, 1, sizeof(c.sp), f);
  file_read(&c.d, 1, sizeof(c.d), f);
  file_read(&c.pc, 1, sizeof(c.pc), f);
  file_read(&c.pbr, 1, 1, f);
  file_read(&c.dbr, 1, 1, f);
  file_read(&c.p, 1, 1, f);
  file_read(&c.emulation, 1, 1, f);
  file_read(&c.waiting, 1, 1, f);
  file_read(&c.stopped, 1, 1, f);
  file_read(&c.faulted, 1, 1, f);
  file_read(&c.faultOpcode, 1, 1, f);
  file_read(&c.nmiVector, 1, sizeof(c.nmiVector), f);
  file_read(wram, 1, sizeof(wram), f);
  file_read(ppu.vram, 1, sizeof(ppu.vram), f);
  file_read(ppu.cgram, 1, sizeof(ppu.cgram), f);
  file_read(ppu.oam, 1, sizeof(ppu.oam), f);
  file_read(&ppu.vramAddress, 1, sizeof(ppu.vramAddress), f);
  file_read(&ppu.cgramAddress, 1, sizeof(ppu.cgramAddress), f);
  file_read(&ppu.oamAddress, 1, sizeof(ppu.oamAddress), f);
  file_read(&ppu.objectControl, 1, 1, f);
  file_read(&ppu.objectEnabled, 1, 1, f);
  file_read(apu.ram, 1, sizeof(apu.ram), f);
  file_read(apu.dsp, 1, sizeof(apu.dsp), f);
  file_read(&apu.dspAddress, 1, 1, f);
  file_read(apu.inputPorts, 1, sizeof(apu.inputPorts), f);
  file_read(&apu.cpu.pc, 1, sizeof(apu.cpu.pc), f);
  file_read(&apu.cpu.sp, 1, sizeof(apu.cpu.sp), f);
  file_read(&apu.cpu.a, 1, 1, f);
  file_read(&apu.cpu.x, 1, 1, f);
  file_read(&apu.cpu.y, 1, 1, f);
  file_read(&apu.cpu.psw, 1, 1, f);
  file_read(&apu.cpu.stopped, 1, 1, f);
  file_read(&apu.cpu.faulted, 1, 1, f);
  file_read(&apu.cpu.faultOpcode, 1, 1, f);
  for (int i = 0; i < 8; ++i) {
    SgbHostApu::VoiceState &v = apu.voices[i];
    file_read(&v.phase, 1, sizeof(v.phase), f);
    file_read(&v.brrAddress, 1, sizeof(v.brrAddress), f);
    file_read(&v.nibble, 1, 1, f);
    file_read(&v.previous1, 1, sizeof(v.previous1), f);
    file_read(&v.previous2, 1, sizeof(v.previous2), f);
    file_read(&v.sample, 1, sizeof(v.sample), f);
    file_read(&v.active, 1, 1, f);
  }
  return true;
}
