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

#include <stdio.h>
#include "common/log.h"
#include "nds_system.h"

#define HEADER_RAM_LOC 0x3FFE00

// this number is chosen arbitrarly currently
#define TICKS_PER_FRAME 0x4000

system_descriptor nds_descriptor = {
    .name = "nds",
    .screen_width = 256,
    .screen_height = 384,
    .open = NULL,
    .close = NULL,
    .frame = NULL
};

arm_memory mmu7_template = {
    .cycles = (cycle_func)nds7_cycles,
    .read_byte = (read_func)nds7_read_byte,
    .read_hword = (read_func)nds7_read_hword,
    .read_word = (read_func)nds7_read_word,
    .write_byte = (write_func)nds7_write_byte,
    .write_hword = (write_func)nds7_write_hword,
    .write_word = (write_func)nds7_write_word,
    .object = NULL
};

void nds7_swi(arm_cpu* cpu, nds_system* system)
{
    LOG(LOG_INFO, "SWI! r15=%x (ARM7)", system->arm7->state->r15);
}

void nds_frame(nds_system* system)
{
    nds_mmu* mmu = system->mmu;

    for (int i = 0; i < TICKS_PER_FRAME; i++) {
        u32 masked7 = mmu->interrupt_enable[ARM7] && mmu->interrupt_flag[ARM7];
        u32 masked9 = mmu->interrupt_enable[ARM9] && mmu->interrupt_flag[ARM9];
        if (mmu->interrupt_master[ARM7] && masked7) {
            LOG(LOG_INFO, "NDS7: IRQ: Triggered with ie&if=0x%x", masked7);
            arm_trigger_irq(system->arm7);
        }
        arm_step(system->arm7);
    }
}

void nds_init_cpu(nds_system* system)
{
    arm_cpu* arm7 = system->arm7;

    // Set NDS7 entrypoint
    arm7->state->r15 = system->cart->header.arm7.entry;

    // Setup stack pointers for USR/SYS, IRQ and SVC
    arm7->state->r[13] = 0x0380FEC0;
    arm7->state->r_irq[0] = 0x0380FFA0;
    arm7->state->r_svc[0] = 0x0380FFC0;

    // Setup SVC handler
    arm7->svc_handler.object = system;
    arm7->svc_handler.method = (arm_svc_call)nds7_swi;

    // Copy MMU template and set underlying object
    arm7->memory = mmu7_template;
    arm7->memory.object = system->mmu;
}

void nds_load_rom(nds_system* system)
{
    nds_mmu* mmu = system->mmu;
    nds_cartridge* cart = system->cart;
    nds_main* binary[2] = {
        &cart->header.arm7,
        &cart->header.arm9
    };

    // Load cartridge header to Main RAM
    memcpy(&mmu->mram[HEADER_RAM_LOC], &cart->header, sizeof(nds_header));

    // Copy executable binaries to RAM (TODO: SWRAM / WRAM7)
    for (int i = 0; i < 2; i++) {
        u32 size = binary[i]->size;
        /*if (size <= BIN_MAX_SIZE_2)*/ {
            int j = 0;
            u8 data[size];
            u32 dest = binary[i]->ram;

            // Get binary from ROM
            fseek(cart->rom_handle, binary[i]->rom, SEEK_SET);
            fread(&data, 1, size, cart->rom_handle);

            // Copy executable data to memory
            while (j < size) {
                if (i == 0) {
                    nds7_write_byte(mmu, dest + j, data[j]);
                } else {
                    //...
                }
                j++;
            }
        } /*else {
            LOG(LOG_ERROR, "NDS%d binary exceeds size limit. NOT loaded.", i == 0 ? 9 : 7);
        }*/
    }
}

void nds_init(nds_system* system)
{
    nds_init_cpu(system);
    nds_load_rom(system);
}

nds_system* nds_make(nds_cartridge* cart)
{
    nds_system* system = malloc(sizeof(nds_system));

    system->arm7 = arm_make(VER_4);
    system->arm9 = arm_make(VER_5);
    system->mmu = nds_make_mmu();
    system->cart = cart;
    nds_init(system);

    return system;
}

void nds_free(nds_system* system)
{
    arm_free(system->arm7);
    arm_free(system->arm9);
    free(system->mmu);
    free(system);
}
