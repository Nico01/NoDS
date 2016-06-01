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

typedef enum {
    ARM7_ALLOC_1ND = 1,
    ARM7_ALLOC_2ND = 2
} nds_swram_alloc;

/*typedef enum {
    ARM7_ALLOC_VRAM_C = 1,
    ARM7_ALLOC_VRAM_D = 2
} nds_vram_alloc;*/

typedef struct {
    int mst;
    int offset;
    bool enable;
} nds_vram_cnt;

typedef struct {
    // IO-registers
    int wramcnt;
    nds_vram_cnt vramcnt[9];

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
