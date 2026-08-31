#include "gameboy.h"
#include "console.h"
#include "inputhelper.h"
#include "menu.h"
#include "romfile.h"
#include "rtc.h"
#include "timer.h"
#include "huc1_rules.h"
#include <stdlib.h>


/* MBC read handlers */

/* MBC3 */
u8 Gameboy::m3r (u16 addr) {
    if (!ramEnabled)
        return 0xff;

    if (currentRamBank >= 0x8 && currentRamBank <= 0xc && !rtcLatched)
        updateClockFromHost();

    switch (currentRamBank) { // Check for RTC register
        case 0x8:
            return rtcLatched ? gbClock.latch[0] : gbClock.mbc3.s;
        case 0x9:
            return rtcLatched ? gbClock.latch[1] : gbClock.mbc3.m;
        case 0xA:
            return rtcLatched ? gbClock.latch[2] : gbClock.mbc3.h;
        case 0xB:
            return rtcLatched ? gbClock.latch[3] : (gbClock.mbc3.d&0xff);
        case 0xC:
            return rtcLatched ? gbClock.latch[4] : gbClock.mbc3.ctrl;
        default: // Not an RTC register
            return getNumSramBanks() ? memory[addr>>12][addr&0xfff] : 0xff;
    }
}

/* MMM01 */
u8 Gameboy::mmm01r(u16 addr) {
    if (!ramEnabled || !getNumSramBanks())
        return 0xff;
    return memory[addr >> 12][addr & 0xfff];
}

/* MBC7 */
u8 Gameboy::m7r (u16 addr) {
    if (!ramEnabled || !mbc7RamEnabled2)
        return 0xff;
    switch (addr & 0xa0f0) {
        case 0xa000:
        case 0xa010:
            return 0xff;
        case 0xa020:
            return mbc7LatchedX & 0xff;
        case 0xa030:
            return mbc7LatchedX >> 8;
        case 0xa040:
            return mbc7LatchedY & 0xff;
        case 0xa050:
            return mbc7LatchedY >> 8;
        case 0xa060:
            return 0;
        case 0xa070:
            return 0xff;
        case 0xa080:
            return mbc7Eeprom.readPins();
        default:
            return 0xff;
    }
}

/* HUC1 */
u8 Gameboy::h1r(u16 addr) {
    if (huc1IrMode)
        return huc1::irRead(false); // No external IR receiver is connected.
    return getNumSramBanks() ? memory[addr >> 12][addr & 0xfff] : 0xff;
}

/* HUC3 */
u8 Gameboy::h3r (u16 addr) {
    switch (HuC3Mode) {
        case 0xc:
            return HuC3Value;
        case 0xb:
        case 0xd:
            /* Return 1 as a fixed value, needed for some games to
             * boot, the meaning is unknown. */
            return 1;
    }
    return (ramEnabled) ? memory[addr>>12][addr&0xfff] : 0xff;
}

/* Game Boy Camera - based off MBC3 */
u8 Gameboy::camr (u16 addr) {
    if (camRegistersEnabled) {
        addr &= 0x7f;
        if (addr == 0x0) return camRegisters[0];
        return 0;
    }

    return memory[addr>>12][addr&0xfff];
}


/* MBC Write handlers */

/* MBC0 (ROM) */
void Gameboy::m0w (u16 addr, u8 val) {
    switch (addr >> 12) {
        case 0x0: /* 0000 - 1fff */
        case 0x1:
            break;
        case 0x2: /* 2000 - 3fff */
        case 0x3:
            break;
        case 0x4: /* 4000 - 5fff */
        case 0x5:
            break;
        case 0x6: /* 6000 - 7fff */
        case 0x7:
            break;
        case 0xa: /* a000 - bfff */
        case 0xb:
            if (getNumSramBanks())
                writeSram(addr&0x1fff, val);
            break;
    }
}

/* MBC1 */
void Gameboy::m1w (u16 addr, u8 val) {
    int newBank;

    switch (addr >> 12) {
        case 0x0: /* 0000 - 1fff */
        case 0x1:
            ramEnabled = ((val & 0xf) == 0xa);
            break;
        case 0x2: /* 2000 - 3fff */
        case 0x3:
            val &= 0x1f;
            if (rockmanMapper)
                newBank = ((val > 0xf) ? val - 8 : val);
            else
                newBank = (romBank & 0xe0) | val;
            refreshRomBank((newBank) ? newBank : 1);
            break;
        case 0x4: /* 4000 - 5fff */
        case 0x5:
            val &= 3;
            /* ROM mode */
            if (memoryModel == 0) {
                newBank = (romBank & 0x1F) | (val<<5);
                refreshRomBank((newBank) ? newBank : 1);
            }
            /* RAM mode */
            else
                refreshRamBank(val);
            break;
        case 0x6: /* 6000 - 7fff */
        case 0x7:
            memoryModel = val & 1;
            break;
        case 0xa: /* a000 - bfff */
        case 0xb:
            if (ramEnabled && getNumSramBanks())
                writeSram(addr&0x1fff, val);
            break;
    }
}

/* MBC2 */
void Gameboy::m2w(u16 addr, u8 val) {
    switch (addr >> 12) {
        case 0x0: /* 0000 - 1fff */
        case 0x1:
            ramEnabled = ((val & 0xf) == 0xa);
            break;
        case 0x2: /* 2000 - 3fff */
        case 0x3:
            refreshRomBank((val) ? val : 1);
            break;
        case 0x4: /* 4000 - 5fff */
        case 0x5:
            break;
        case 0x6: /* 6000 - 7fff */
        case 0x7:
            break;
        case 0xa: /* a000 - bfff */
        case 0xb:
            if (ramEnabled && getNumSramBanks() && addr < 0xa200)
                writeSram(addr&0x1fff, val&0xf);
            break;
    }
}

/* MBC3 */
void Gameboy::m3w(u16 addr, u8 val) {
    switch (addr >> 12) {
        case 0x0: /* 0000 - 1fff */
        case 0x1:
            ramEnabled = ((val & 0xf) == 0xa);
            break;
        case 0x2: /* 2000 - 3fff */
        case 0x3:
            val &= 0x7f;
            refreshRomBank((val) ? val : 1);
            break;
        case 0x4: /* 4000 - 5fff */
        case 0x5:
            /* The RTC register is selected by writing values 0x8-0xc, ram banks
             * are selected by values 0x0-0x3 */
            if (val <= 0x3)
                refreshRamBank(val);
            else if (val >= 8 && val <= 0xc)
                currentRamBank = val;
            break;
        case 0x6: /* 6000 - 7fff */
        case 0x7:
            val &= 1;
            if (rtcLatchState == 0 && val == 1)
                latchClock();
            rtcLatchState = val;
            break;
        case 0xa: /* a000 - bfff */
        case 0xb:
            if (!ramEnabled)
                break;

            if (currentRamBank >= 0x8 && currentRamBank <= 0xc)
                updateClockFromHost();

            switch (currentRamBank) { // Check for RTC register
                case 0x8:
                    if (val > 59)
                        val = 59;
                    if (gbClock.mbc3.s != val) {
                        gbClock.mbc3.s = val;
                        writeClockStruct();
                    }
                    return;
                case 0x9:
                    if (val > 59)
                        val = 59;
                    if (gbClock.mbc3.m != val) {
                        gbClock.mbc3.m = val;
                        writeClockStruct();
                    }
                    return;
                case 0xA:
                    if (val > 23)
                        val = 23;
                    if (gbClock.mbc3.h != val) {
                        gbClock.mbc3.h = val;
                        writeClockStruct();
                    }
                    return;
                case 0xB:
                    if ((gbClock.mbc3.d&0xff) != val) {
                        gbClock.mbc3.d &= 0x100;
                        gbClock.mbc3.d |= val;
                        writeClockStruct();
                    }
                    return;
                case 0xC:
                    val &= 0xc1;
                    if (gbClock.mbc3.ctrl != val) {
                        gbClock.mbc3.d &= 0xFF;
                        gbClock.mbc3.d |= (val&1)<<8;
                        gbClock.mbc3.ctrl = val;
                        writeClockStruct();
                    }
                    return;
                default: // Not an RTC register
                    if (getNumSramBanks())
                        writeSram(addr&0x1fff, val);
            }
            break;
    }
}

void Gameboy::writeClockStruct() {
    if (autoSavingEnabled && saveFile != NULL) {
        file_seek(saveFile, getNumSramBanks()*0x2000, SEEK_SET);
        file_write(&gbClock, 1, sizeof(gbClock), saveFile);
        saveModified = true;
    }
}


/* MBC5 */
void Gameboy::m5w (u16 addr, u8 val) {
    switch (addr >> 12) {
        case 0x0: /* 0000 - 1fff */
        case 0x1:
            ramEnabled = ((val & 0xf) == 0xa);
            break;
        case 0x2: /* 2000 - 3fff */
            refreshRomBank((romBank & 0x100) |  val);
            break;
        case 0x3:
            refreshRomBank((romBank & 0xff ) | (val&1) << 8);
            break;
        case 0x4: /* 4000 - 5fff */
        case 0x5:
            val &= 0xf;
            /* MBC5 might have a rumble motor, which is triggered by the
             * 4th bit of the value written */
            if (romFile->hasRumble()) {
                if (rumbleStrength) {
                    if (rumbleInserted) {
                        rumbleValue = (val & 0x8) ? 1 : 0;
                        if (rumbleValue != lastRumbleValue)
                        {
                            system_doRumble(rumbleValue);
                            lastRumbleValue = rumbleValue;
                        }
                    }
                }

                val &= 0x07;
            }

            refreshRamBank(val);
            break;
        case 0x6: /* 6000 - 7fff */
        case 0x7:
            break;
        case 0xa: /* a000 - bfff */
        case 0xb:
            if (ramEnabled && getNumSramBanks())
                writeSram(addr&0x1fff, val);
            break;
    }
}

/* MBC7 */
void Gameboy::m7w (u16 addr, u8 val) {
    switch (addr >> 12) {
        case 0x0: /* 0000 - 1fff */
        case 0x1:
            ramEnabled = ((val & 0xf) == 0xa);
            break;
        case 0x2: /* 2000 - 3fff */
        case 0x3:
            refreshRomBank(val & 0x7f);
            break;
        case 0x4: /* 4000 - 5fff */
        case 0x5:
            mbc7RamEnabled2 = (val == 0x40);
            break;
        case 0x6: /* 6000 - 7fff */
        case 0x7:
            break;
        case 0xa: /* a000 - bfff */
        case 0xb:
            if (!ramEnabled || !mbc7RamEnabled2)
                break;
            switch (addr & 0xa0f0) {
            case 0xa000:
                if (val == 0x55) {
                    mbc7LatchedX = mbc7LatchedY = 0x8000;
                    mbc7LatchReady = true;
                }
                break;
            case 0xa010:
                if (val == 0xaa && mbc7LatchReady) {
                    mbc7LatchedX = system_getMotionSensorX();
                    mbc7LatchedY = system_getMotionSensorY();
                    mbc7LatchReady = false;
                }
                break;
            case 0xa080:
                mbc7Eeprom.writePins(val, externRam, 256);
                if (mbc7Eeprom.consumeModified() && autoSavingEnabled) {
                    saveModified = true;
                    if (fatBytesPerSector > 0)
                        dirtySectors[0] = true;
                    ++numSaveWrites;
                }
                break;
            }
            break;
    }
}

/* HUC1 */
void Gameboy::h1w(u16 addr, u8 val) {
    switch (addr >> 12) {
        case 0x0: /* 0000 - 1fff */
        case 0x1:
            huc1IrMode = huc1::irMode(val);
            ramEnabled = !huc1IrMode;
            break;
        case 0x2: /* 2000 - 3fff */
        case 0x3:
            refreshRomBank(huc1::romBank(val));
            break;
        case 0x4: /* 4000 - 5fff */
        case 0x5:
            refreshRamBank(huc1::ramBank(val));
            break;
        case 0x6: /* 6000 - 7fff */
        case 0x7:
            break;
        case 0xa: /* a000 - bfff */
        case 0xb:
            if (huc1IrMode)
                huc1IrOutput = val & 1;
            else if (getNumSramBanks())
                writeSram(addr&0x1fff, val);
            break;
    }
}

/* HUC3 */

void Gameboy::h3w (u16 addr, u8 val) {
    switch (addr >> 12) {
        case 0x0: /* 0000 - 1fff */
        case 0x1:
            ramEnabled = ((val & 0xf) == 0xa);
            HuC3Mode = val;
            break;
        case 0x2: /* 2000 - 3fff */
        case 0x3:
            refreshRomBank((val) ? val : 1);
            break;
        case 0x4: /* 4000 - 5fff */
        case 0x5:
            refreshRamBank(val & 0xf);
            break;
        case 0x6: /* 6000 - 7fff */
        case 0x7:
            break;
        case 0xa: /* a000 - bfff */
        case 0xb:
            switch (HuC3Mode) {
                case 0xb:
                    handleHuC3Command(val);
                    break;
                case 0xc:
                case 0xd:
                case 0xe:
                    break;
                default:
                    if (ramEnabled && getNumSramBanks())
                        writeSram(addr&0x1fff, val);
            }
            break;
    }
}

void Gameboy::handleHuC3Command (u8 cmd) 
{
    switch (cmd&0xf0) {
        case 0x10: /* Read clock */
            if (HuC3Shift > 24)
                break;

            switch (HuC3Shift) {
                case 0: case 4: case 8:     /* Minutes */
                    HuC3Value = (gbClock.huc3.m >> HuC3Shift) & 0xf;
                    break;
                case 12: case 16: case 20:  /* Days */
                    HuC3Value = (gbClock.huc3.d >> (HuC3Shift - 12)) & 0xf;
                    break;
                case 24:                    /* Year */
                    HuC3Value = gbClock.huc3.y & 0xf;
                    break;
            }
            HuC3Shift += 4;
            break;
        case 0x40:
            switch (cmd&0xf) {
                case 0: case 4: case 7:
                    HuC3Shift = 0;
                    break;
            }

            latchClock();
            break;
        case 0x50:
            break;
        case 0x60: 
            HuC3Value = 1;
            break;
        default:
            printLog("unhandled HuC3 cmd %02x\n", cmd);
    }
}


/* Game Boy Camera - based off MBC3 */
#define GBCAM_RAM_PICT_SIZE (14 * 16 * 16)

void Gameboy::camw (u16 addr, u8 val) {
    switch (addr >> 12) {
        case 0x0: /* 0000 - 1fff */
        case 0x1:
            ramEnabled = ((val & 0xf) == 0xa);
            break;
        case 0x2: /* 2000 - 3fff */
        case 0x3:
            val &= 0x7f;
            refreshRomBank((val) ? val : 1);
            break;
        case 0x4: /* 4000 - 5fff */
        case 0x5:
            if (val < 0x10) {
                refreshRamBank(val);
                camRegistersEnabled = false;
                ramEnabled = true;
            } else {
                camRegistersEnabled = true;
                ramEnabled = false;
            }
            break;
        case 0x6: /* 6000 - 7fff */
        case 0x7:
            break;
        case 0xa: /* a000 - bfff */
        case 0xb:
            if (camRegistersEnabled) {
                addr &= 0x7f;
                if (addr < sizeof(camRegisters)) {
                    if(addr == 0x0 && val & 1) {
                        /* start capture -- copy image to ram */
                        val &= 6; // timing hack?
                        camRegisters[addr] = val;
                        u8* gbcamData = (u8*)malloc(GBCAM_RAM_PICT_SIZE);
                        memset(gbcamData, 0, GBCAM_RAM_PICT_SIZE);
                        system_getCamera(gbcamData, camRegisters);
                        memcpy(externRam+0x100, gbcamData, GBCAM_RAM_PICT_SIZE);
                        free(gbcamData);
                    } else {
                        camRegisters[addr] = val;
                    }
                }
            } else {
                if (ramEnabled && getNumSramBanks())
                    writeSram(addr&0x1fff, val);
            }
            break;
    }
}

void Gameboy::updateClockFromHost()
{
    const time_t now = getTime();
    if (gbClock.last <= 0 || now < gbClock.last) {
        gbClock.last = now;
        return;
    }

    const uint64_t elapsed = static_cast<uint64_t>(now - gbClock.last);
    switch (romFile->getMBC()) {
        case MBC3:
            rtc::advanceMbc3(gbClock.mbc3.s, gbClock.mbc3.m,
                    gbClock.mbc3.h, gbClock.mbc3.d,
                    gbClock.mbc3.ctrl, elapsed);
            break;
        case HUC3:
            rtc::advanceHuc3(gbClock.huc3.m, gbClock.huc3.d,
                    gbClock.huc3.y, elapsed);
            break;
    }
    gbClock.last = now;
}

/* MMM01 */
void Gameboy::mmm01w(u16 addr, u8 val) {
    if (addr >= 0xa000) {
        if (ramEnabled && getNumSramBanks())
            writeSram(addr & 0x1fff, val);
        return;
    }

    if (addr < 0x8000) {
        mmm01.write(addr, val);
        refreshMmm01Banks();
    }
}

void Gameboy::latchClock()
{
    updateClockFromHost();

    if (romFile->getMBC() == MBC3) {
        gbClock.latch[0] = gbClock.mbc3.s;
        gbClock.latch[1] = gbClock.mbc3.m;
        gbClock.latch[2] = gbClock.mbc3.h;
        gbClock.latch[3] = gbClock.mbc3.d & 0xff;
        gbClock.latch[4] = gbClock.mbc3.ctrl;
        rtcLatched = true;
    }
}
