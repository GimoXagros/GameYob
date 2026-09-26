#include "sgb_host.h"
#include <assert.h>
#include <stdio.h>

void file_read(void *, int, int, FileHandle *) {}
void file_write(const void *, int, int, FileHandle *) {}

int main() {
    SgbHost host;
    for (int frame = 0; frame < 4; ++frame)
        host.runFrame();
    // A border-only cartridge has not transferred a host CPU/APU program.
    // Its SGB packet/border pipeline remains active, but no host instructions
    // should be fabricated from zero-filled reset memory every GB frame.
    if (host.apu.cpu.pc != 0 || !host.apu.cpu.stopped ||
            !host.cpu.state().stopped) {
        fprintf(stderr,
            "first mismatch phase=idle_host_tick apu_pc=%u apu_stopped=%u cpu_stopped=%u\n",
            host.apu.cpu.pc, host.apu.cpu.stopped,
            host.cpu.state().stopped);
        return 1;
    }

    const unsigned char program[] = {0, 0, 0, 2};
    assert(host.apu.transferProgram(program, sizeof(program)));
    assert(!host.apu.cpu.stopped);
    host.runFrame();
    assert(host.apu.cpu.pc != 0x200);
    host.wram[0x100] = 0xdb; // STP: prove a transferred JUMP starts execution.
    host.jump(0x7e0100, 0);
    assert(host.cpu.state().stopped && host.cpu.state().pc == 0x101);
    return 0;
}
