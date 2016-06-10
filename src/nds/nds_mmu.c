/*
 * Copyright (C) 2016 Frederic Meyer
 *
 * This file is part of NoDS.
 *
 * NoDS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * NoDS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with NoDS. If not, see <http://www.gnu.org/licenses/>.
 */

#include "../common/log.h"
#include "nds_mmu.h"

nds_mmu* nds_make_mmu()
{
    nds_mmu* mmu = calloc(1, sizeof(nds_mmu));

    // Apparently the NDS7 core has access to both
    // SWRAM pages from the very beginning. Though
    // I'll have to do further investigation on this.
    mmu->wramcnt = ARM7_ALLOC_1ND | ARM7_ALLOC_2ND;

    return mmu;
}

int nds7_cycles(nds_mmu* mmu, u32 address, arm_size size, bool write, arm_cycle type)
{
    return 1;
}

u8 nds7_read_byte(nds_mmu* mmu, u32 address)
{
    int page = address >> 24;
    address &= 0x00FFFFFF;

    switch (page) {
    case 2:
        return mmu->mram[address % 0x400000];
    case 3:
        // Distinguish between WRAM7 area and SWRAM area.
        if (address >= 0x800000) {
            return mmu->wram7[(address - 0x800000) % 0x10000];
        }

        // The SWRAM consists of two 16KB pages. The ARM7 core
        // can have either one of them or both mapped to his memory.
        // If none of the pages are mapped the WRAM7 is mirrored.
        switch (mmu->wramcnt) {
        case 0:
            return mmu->wram7[address % 0x10000];
        case ARM7_ALLOC_1ND:
            return mmu->swram[address % 0x4000];
        case ARM7_ALLOC_2ND:
            return mmu->swram[address % 0x4000 + 0x4000];
        case ARM7_ALLOC_1ND|ARM7_ALLOC_2ND:
            return mmu->swram[address % 0x8000];
        }

        return 0;
    case 4: {
        LOG(LOG_INFO, "IO: read register %x (NDS7)", address);

        switch (address) {
        case NDS7_IO_SPICNT: {
            nds_spi_bus* spi_bus = &mmu->spi_bus;

            return spi_bus->baud_rate |
                   (spi_bus->busy ? 128 : 0);
        }
        case NDS7_IO_SPICNT+1: {
            nds_spi_bus* spi_bus = &mmu->spi_bus;

            return spi_bus->device |
                   (spi_bus->bugged ? 4 : 0) |
                   (spi_bus->chipselect ? 8 : 0) |
                   (spi_bus->ireq ? 64 : 0) |
                   (spi_bus->enable ? 128 : 0);
        }
        }
        return 0;
    }
    }

    LOG(LOG_ERROR, "Read byte from %x", (page << 24) | address);

    return 0;
}

u16 nds7_read_hword(nds_mmu* mmu, u32 address)
{
    return nds7_read_byte(mmu, address) |
           (nds7_read_byte(mmu, address+1) << 8);
}

u32 nds7_read_word(nds_mmu* mmu, u32 address)
{
    return nds7_read_byte(mmu, address) |
           (nds7_read_byte(mmu, address+1) << 8) |
           (nds7_read_byte(mmu, address+2) << 16) |
           (nds7_read_byte(mmu, address+3) << 24);
}

void nds7_write_byte(nds_mmu* mmu, u32 address, u8 value)
{
    int page = address >> 24;
    address &= 0x00FFFFFF;

    switch (page) {
    case 2:
        mmu->mram[address % 0x400000] = value;
        break;
    case 3:
        // Distinguish between WRAM7 area and SWRAM area.
        if (address >= 0x800000) {
            mmu->wram7[(address - 0x800000) % 0x10000] = value;
            break;
        }

        // See nds7_read_byte for further explanation
        switch (mmu->wramcnt) {
        case 0:
            mmu->wram7[address % 0x10000] = value;
            break;
        case ARM7_ALLOC_1ND:
            mmu->swram[address % 0x4000] = value;
            break;
        case ARM7_ALLOC_2ND:
            mmu->swram[address % 0x4000 + 0x4000] = value;
            break;
        case ARM7_ALLOC_1ND|ARM7_ALLOC_2ND:
            mmu->swram[address % 0x8000] = value;
            break;
        }

        break;
    case 4: {
        LOG(LOG_INFO, "IO: write register %x=%x (NDS7)", address, value);

        switch (address) {
        case NDS7_IO_SPICNT: {
            nds_spi_bus* spi_bus = &mmu->spi_bus;

            // TODO: busy flag is presumably read-only, further
            //       investigation / reversing required.
            spi_bus->baud_rate = value & 3;
            break;
        }
        case NDS7_IO_SPICNT+1: {
            nds_spi_bus* spi_bus = &mmu->spi_bus;

            spi_bus->device = value & 3;
            spi_bus->bugged = value & 4;
            spi_bus->chipselect = value & 8;
            spi_bus->ireq = value & 64;
            spi_bus->enable = value & 128;
            break;
        }
        }
        break;
    }
    default:
        LOG(LOG_ERROR, "Write byte to %x=%x", (page << 24) | address, value);
    }
}

void nds7_write_hword(nds_mmu* mmu, u32 address, u16 value)
{
    nds7_write_byte(mmu, address, value & 0xFF);
    nds7_write_byte(mmu, address + 1, value >> 8);
}

void nds7_write_word(nds_mmu* mmu, u32 address, u32 value)
{
    nds7_write_byte(mmu, address, value & 0xFF);
    nds7_write_byte(mmu, address + 1, (value >> 8) & 0xFF);
    nds7_write_byte(mmu, address + 2, (value >> 16) & 0xFF);
    nds7_write_byte(mmu, address + 3, (value >> 24) & 0xFF);
}
