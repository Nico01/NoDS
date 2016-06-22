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

#ifndef _NDS_MMU_H_
#define _NDS_MMU_H_

#include "../common/types.h"
#include "../arm/arm_cpu.h"
#include "nds_spi.h"

#define FIFO_SIZE 16

typedef enum {
    ARM7 = 0,
    ARM9 = 1
} nds_cpu_index;

// fix name inconsisties
typedef enum {
    NDS_IPCSYNC = 0x180,
    NDS_IPCFIFOCNT = 0x184,
    NDS_IPCFIFOSEND = 0x188,
    NDS_IPCFIFORECV = 0x100000,
    NDS7_IO_SPICNT = 0x1C0,
    NDS7_IO_SPIDATA = 0x1C2,
    NDS_IO_IME = 0x208,
    NDS_IO_IE = 0x210,
    NDS_IO_IF = 0x214,
    NDS7_VRAMSTAT = 0x240,
    NDS7_WRAMSTAT = 0x241
} nds_io_reg;

// move to another file
typedef enum {
    INT_VBLANK = 1,
    INT_HBLANK = 2,
    INT_VCOUNT = 4,
    INT_TIMER0 = 8,
    INT_TIMER1 = 16,
    INT_TIMER2 = 32,
    INT_TIMER3 = 64,
    INT_RTC = 128,
    INT_DMA0 = 256,
    INT_DMA1 = 512,
    INT_DMA2 = 1024,
    INT_DMA3 = 2048,
    INT_KEYPAD = 4096,
    INT_SLOT2 = 8192,
    INT_IPC_SYNC = 65536,
    INT_IPC_SEND = 131072,
    INT_IPC_RECV = 262144,
    INT_SLOT1_COMPLETE = 524288,
    INT_SLOT1_IREQ_MC = 1048576,
    INT_GEOMETRY_FIFO = 2097152,
    INT_UNFOLD_SCREEN = 4194304,
    INT_SPI_BUS = 8388608,
    INT_WIFI = 16777216
} nds_interrupt;

typedef enum {
    ARM7_ALLOC_1ND = 1,
    ARM7_ALLOC_2ND = 2
} nds_swram_alloc;

typedef struct {
    int mst;
    int offset;
    bool enable;
} nds_vram_cnt;

typedef struct {
    u8 data_in;
    bool allow_irq;
} nds_ipc_sync;

typedef struct {
    bool enable;
    bool enable_irq_send;
    bool enable_irq_recv;
    bool error;
} nds_fifo_cnt;

typedef struct {
    u32 buffer[FIFO_SIZE];
    u32 recent_read;
    int write_index;
} nds_fifo;

typedef struct {
    // Memory Control
    int wramcnt;
    nds_vram_cnt vramcnt[9];

    // Interrupt Control
    u32 interrupt_master[2];
    u32 interrupt_enable[2];
    u32 interrupt_flag[2];

    // IPC SYNC registers
    nds_ipc_sync sync[2];

    // IPC (RECV) FIFOS
    nds_fifo_cnt fifocnt[2];
    nds_fifo fifo[2];

    // Serial Peripheral Interface (SPI)
    nds_spi_bus spi_bus;

    u8 mram[0x400000]; // 4MB Main Memory
    u8 swram[0x8000]; // 32KB Shared WRAM
    u8 wram7[0x10000]; // 64KB ARM7 WRAM

    // Video RAM
    u8 vram_a[0x20000]; // 128KB
    u8 vram_b[0x20000]; // 128KB
    u8 vram_c[0x20000]; // 128KB
    u8 vram_d[0x20000]; // 128KB
    u8 vram_e[0x10000]; // 64KB
    u8 vram_f[0x4000]; // 16KB
    u8 vram_g[0x4000]; // 16KB
    u8 vram_h[0x8000]; // 32KB
    u8 vram_i[0x4000]; // 16KB
} nds_mmu;

nds_mmu* nds_make_mmu();

int nds7_cycles(nds_mmu* mmu, u32 address, arm_size size, bool write, arm_cycle type);
u8 nds7_read_byte(nds_mmu* mmu, u32 address);
u16 nds7_read_hword(nds_mmu* mmu, u32 address);
u32 nds7_read_word(nds_mmu* mmu, u32 address);
void nds7_write_byte(nds_mmu* mmu, u32 address, u8 value);
void nds7_write_hword(nds_mmu* mmu, u32 address, u16 value);
void nds7_write_word(nds_mmu* mmu, u32 address, u32 value);

#endif
