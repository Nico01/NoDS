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

#include <stdlib.h>
#include "../common/log.h"
#include "nds_mmu.h"

nds_mmu* nds_make_mmu()
{
    nds_mmu* mmu = calloc(1, sizeof(nds_mmu));

    // Apparently the NDS7 core has access to both
    // SWRAM pages from the very beginning. Though
    // I'll have to do further investigation on this.
    mmu->wramcnt = ARM7_ALLOC_1ND | ARM7_ALLOC_2ND;

    // Initialize SPI master and slaves
    nds_spi_init(&mmu->spi_bus);

    return mmu;
}

inline u32 nds7_fifo_recv(nds_mmu* mmu)
{
    nds_fifo* fifo = &mmu->fifo[ARM7];
    int write_index = fifo->write_index;
    u32* buffer = fifo->buffer;
    u8 value;

    // Reading from empty FIFO results in returning
    // the most recent read FIFO word and setting
    // the error flag.
    if (write_index == 0) {
        mmu->fifocnt[ARM7].error = true;
        return fifo->recent_read;
    }

    value = fifo->recent_read = buffer[0];

    // Only if the FIFO is enabled the oldest
    // FIFO word also gets removed from the FIFO.
    if (mmu->fifocnt[ARM7].enable) {
        for (int i = 1; i < write_index; i++) {
            buffer[i - 1] = buffer[i];
        }

        fifo->write_index--;
    }

    return value;
}

inline void nds7_fifo_send(nds_mmu* mmu, u32 value)
{
    nds_fifo* fifo = &mmu->fifo[ARM9];

    // Writing when the FIFO is full results in
    // the error flag being set and no writing happening.
    if (fifo->write_index == FIFO_SIZE) {
        mmu->fifocnt[ARM7].error = true;
        return;
    }

    if (mmu->fifocnt[ARM7].enable) {
        fifo->buffer[fifo->write_index++] = value;
    }
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
        case NDS_IPCSYNC:
            LOG(LOG_INFO, "IPC: SYNC: read input (%x) (NDS7)", mmu->sync[ARM7].data_in);
            return mmu->sync[ARM7].data_in;
        case NDS_IPCSYNC+1:
            return mmu->sync[ARM9].data_in |
                   (mmu->sync[ARM7].allow_irq ? 64 : 0);
        case NDS_IPCFIFOCNT: {
            nds_fifo* send_fifo = &mmu->fifo[ARM9];
            nds_fifo_cnt* fifocnt = &mmu->fifocnt[ARM7];

            return ((send_fifo->write_index == 0) ? 1 : 0) |
                   ((send_fifo->write_index == FIFO_SIZE) ? 2 : 0) |
                   (fifocnt->enable_irq_send ? 4 : 0);
        }
        case NDS_IPCFIFOCNT+1: {
            nds_fifo* recv_fifo = &mmu->fifo[ARM7];
            nds_fifo_cnt* fifocnt = &mmu->fifocnt[ARM7];

            return ((recv_fifo->write_index == 0) ? 1 : 0) |
                   ((recv_fifo->write_index == FIFO_SIZE) ? 2 : 0) |
                   (fifocnt->enable_irq_recv ? 4 : 0) |
                   (fifocnt->error ? 128 : 0) |
                   (fifocnt->enable ? 256 : 0);
        }
        case NDS_IPCFIFORECV:
        case NDS_IPCFIFORECV+1:
        case NDS_IPCFIFORECV+2:
        case NDS_IPCFIFORECV+3:
            LOG(LOG_ERROR, "IPC: FIFO: non-standard fifo read. unsupported. (NDS7)");
            return 0;
        case NDS7_IO_SPICNT: {
            nds_spi_bus* spi_bus = &mmu->spi_bus;

            return spi_bus->baud_rate |
                   (spi_bus->busy ? 128 : 0);
        }
        case NDS7_IO_SPICNT+1: {
            nds_spi_bus* spi_bus = &mmu->spi_bus;

            return spi_bus->device |
                   (spi_bus->bugged ? 4 : 0) |
                   (spi_bus->cs_hold ? 8 : 0) |
                   (spi_bus->ireq ? 64 : 0) |
                   (spi_bus->enable ? 128 : 0);
        }
        case NDS7_IO_SPIDATA: {
            nds_spi_bus* spi_bus = &mmu->spi_bus;

            return nds_spi_read(spi_bus);
        }
        case NDS7_IO_SPIDATA+1:
            return 0;
        case NDS_IO_IME:
            return mmu->interrupt_master[0] & 0xFF;
        case NDS_IO_IME+1:
            return (mmu->interrupt_master[0] >> 8) & 0xFF;
        case NDS_IO_IME+2:
            return (mmu->interrupt_master[0] >> 16) & 0xFF;
        case NDS_IO_IME+3:
            return mmu->interrupt_master[0] >> 24;
        case NDS_IO_IE:
            return mmu->interrupt_enable[0] & 0xFF;
        case NDS_IO_IE+1:
            return (mmu->interrupt_enable[0] >> 8) & 0xFF;
        case NDS_IO_IE+2:
            return (mmu->interrupt_enable[0] >> 16) & 0xFF;
        case NDS_IO_IE+3:
            return mmu->interrupt_enable[0] >> 24;
        case NDS_IO_IF:
            return mmu->interrupt_flag[0] & 0xFF;
        case NDS_IO_IF+1:
            return (mmu->interrupt_flag[0] >> 8) & 0xFF;
        case NDS_IO_IF+2:
            return (mmu->interrupt_flag[0] >> 16) & 0xFF;
        case NDS_IO_IF+3:
            return mmu->interrupt_flag[0] >> 24;
        case NDS7_VRAMSTAT:
            return (mmu->vramcnt[2].enable && mmu->vramcnt[2].mst == 2) |
                   (mmu->vramcnt[3].enable && mmu->vramcnt[3].mst == 2);
        case NDS7_WRAMSTAT:
            return mmu->wramcnt;
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
    int page = address >> 24;

    if (page == 4) {
        switch (address & 0x00FFFFFF) {
        case NDS_IPCFIFORECV: {
            u32 value = nds7_fifo_recv(mmu);
            LOG(LOG_INFO, "IPC: FIFO: dequeued 0x%x (NDS7)", value);
            return value;
        }
        }
    }

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
        case NDS_IPCSYNC+1:
            LOG(LOG_INFO, "IPC: SYNC: write output (%x) (NDS7)", value & 0xF);

            mmu->sync[ARM7].allow_irq = value & 64;
            mmu->sync[ARM9].data_in = value & 0xF;

            // Trigger SYNC interrupt on remote cpu if neccessary
            if ((value & 32) && mmu->sync[ARM9].allow_irq) {
                mmu->interrupt_flag[ARM9] |= INT_IPC_SYNC;
                LOG(LOG_INFO, "IPC: SYNC: generate remote IRQ (NDS7)");
            }
            break;
        case NDS_IPCFIFOCNT: {
            nds_fifo* send_fifo = &mmu->fifo[ARM9];
            nds_fifo_cnt* fifocnt = &mmu->fifocnt[ARM7];

            fifocnt->enable_irq_send = value & 4;

            if (value & (1 << 3)) {
                send_fifo->write_index = 0;
                send_fifo->recent_read = 0; // not sure
            }

            break;
        }
        case NDS_IPCFIFOCNT+1: {
            nds_fifo_cnt* fifocnt = &mmu->fifocnt[ARM7];

            fifocnt->enable_irq_recv = value & 4;
            fifocnt->enable = value & 256;

            // acknowledge errors
            if (value & 128) {
                fifocnt->error = false;
            }
            break;
        }
        case NDS_IPCFIFOSEND:
        case NDS_IPCFIFOSEND+1:
        case NDS_IPCFIFOSEND+2:
        case NDS_IPCFIFOSEND+3:
            LOG(LOG_ERROR, "IPC: FIFO: non-standard fifo write. unsupported. (NDS7)");
            break;
        case NDS7_IO_SPICNT: {
            nds_spi_bus* spi_bus = &mmu->spi_bus;

            // TODO: busy flag is presumably read-only, further
            //       investigation / reversing required.
            spi_bus->baud_rate = value & 3;
            break;
        }
        case NDS7_IO_SPICNT+1: {
            nds_spi_bus* spi_bus = &mmu->spi_bus;

            spi_bus->device_old = spi_bus->device;
            spi_bus->device = value & 3;
            spi_bus->bugged = value & 4;
            spi_bus->cs_hold = value & 8;
            spi_bus->ireq = value & 64;
            spi_bus->enable = value & 128;
            nds_spi_update_cs(spi_bus); // register changes in "chipselect" regarding firmware.
            break;
        }
        case NDS7_IO_SPIDATA: {
            nds_spi_bus* spi_bus = &mmu->spi_bus;

            nds_spi_write(spi_bus, value);

            // triggers IRQ on transfer completion if specified.
            // this should propably be moved into nds_spi_write
            // but this is sooo much simpler ._.
            if (spi_bus->ireq) {
                mmu->interrupt_flag[0] |= INT_SPI_BUS;
            }
        }
        case NDS_IO_IME:
        case NDS_IO_IME+1:
        case NDS_IO_IME+2:
        case NDS_IO_IME+3: {
            int n = (address - NDS_IO_IME) * 8;
            mmu->interrupt_master[0] = (mmu->interrupt_master[0] & ~(0xFF << n)) | (value << n);
            break;
        }
        case NDS_IO_IE:
        case NDS_IO_IE+1:
        case NDS_IO_IE+2:
        case NDS_IO_IE+3: {
            int n = (address - NDS_IO_IE) * 8;
            mmu->interrupt_enable[0] = (mmu->interrupt_enable[0] & ~(0xFF << n)) | (value << n);
            break;
        }
        case NDS_IO_IF:
        case NDS_IO_IF+1:
        case NDS_IO_IF+2:
        case NDS_IO_IF+3: {
            int n = (address - NDS_IO_IF) * 8;
            mmu->interrupt_flag[0] &= ~(value << n);
            break;
        }
        }
        break;
    }
    // NoDS debug port (FFXXXXXXh)
    case 255:
        printf("%c", value);
        break;
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
    int page = address >> 24;

    if (page == 4) {
        switch (address & 0x00FFFFFF) {
        case NDS_IPCFIFOSEND:
            nds7_fifo_send(mmu, value);
            LOG(LOG_INFO, "IPC: FIFO: enqueue 0x%x (NDS7)", value);
            return;
        }
    }

    nds7_write_byte(mmu, address, value & 0xFF);
    nds7_write_byte(mmu, address + 1, (value >> 8) & 0xFF);
    nds7_write_byte(mmu, address + 2, (value >> 16) & 0xFF);
    nds7_write_byte(mmu, address + 3, (value >> 24) & 0xFF);
}
