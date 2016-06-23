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

#ifndef _NDS_CARTRIDGE_H_
#define _NDS_CARTRIDGE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common/types.h"

#define NDS_STRING(str, len) strcpy(calloc(len + 1, 1), str)

#pragma pack(push, r1, 1)

typedef enum {
    CART_INVALID,
    CART_HOMEBREW,
    CART_MULTIBOOT,
    CART_DUMPED,
    CART_ENCRYPTED
} nds_cart_type;

typedef struct {
    u32 rom;
    u32 entry;
    u32 ram;
    u32 size;
} nds_main;

typedef struct {
    u32 offset;
    u32 size;
} nds_block;

typedef struct {
    char game_title[12];
    char game_code[4];
    char maker_code[2];
    u8 unit_code;
    u8 encryption_seed;
    u8 capacity;
    u8 reserved1[8];
    u8 region;
    u8 version;
    u8 autostart;
    nds_main arm9;
    nds_main arm7;
    nds_block fnt;
    nds_block fat;
    nds_block arm9_overlay;
    nds_block arm7_overlay;
    u32 port1A4;
    u32 port1A4_key1;
    u32 icon_offset;
    u16 secure_checksum;
    u16 secure_delay;
    u32 arm9_auto;
    u32 arm7_auto;
    u8 secure_disable[8];
    u32 rom_size;
    u32 header_size;
    u8 reserved2[0x38];
    u8 logo[0x9C];
    u16 logo_checksum;
    u16 header_checksum;
    
    struct {
        u32 rom;
        u32 size;
        u32 ram;
    } debug;
    
    u32 reserved3;
    u8 reserved4[0x90];
} nds_header;

#pragma pack(pop, r1)

typedef struct {
    nds_header header;
    nds_cart_type type;
    FILE* rom_handle;
} nds_cartridge;

nds_cartridge* nds_cart_open(char* rom_path);
void nds_cart_close(nds_cartridge* cart);

#endif
