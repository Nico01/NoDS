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

#ifndef _NDS_SYSTEM_H_
#define _NDS_SYSTEM_H_

#include "../arm/arm_cpu.h"
#include "../nds/nds_mmu.h"
#include "../nds/nds_cartridge.h"

typedef struct {
    arm_cpu* arm7;
    arm_cpu* arm9;
    nds_mmu* mmu;
    nds_cartridge* cart;
} nds_system;

void nds_init(nds_system* system);
nds_system* nds_make(nds_cartridge* cart);
void nds_free(nds_system* system);

#endif
