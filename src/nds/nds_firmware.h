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

#ifndef _NDS_FIRMWARE_H_
#define _NDS_FIRMWARE_H_

#include <stdio.h>
#include "../common/types.h"

typedef enum {
    FIRM_CMD_WREN = 0x06,
    FIRM_CMD_WRDI = 0x04,
    FIRM_CMD_RDID = 0x9F,
    FIRM_CMD_RDSR = 0x05,
    FIRM_CMD_READ = 0x03,
    FIRM_CMD_FAST = 0x0B,
    FIRM_CMD_PW = 0x0A,
    FIRM_CMD_PP = 0x02,
    FIRM_CMD_PE = 0xDB,
    FIRM_CMD_SE = 0xD8,
    FIRM_CMD_DP = 0xB9,
    FIRM_CMD_RDP = 0xAB
} nds_firmware_cmd;

typedef enum {
    FIRM_STAT_IDLE = 0,
    FIRM_STAT_CMD = 1,
    FIRM_STAT_ADDR = 2,
    FIRM_STAT_JEDEC = 4,
    FIRM_STAT_STATUS = 8,
    FIRM_STAT_READ = 16,
    FIRM_STAT_DUMMY = 32,
    FIRM_STAT_DEEP = 64
} nds_firmware_state;

typedef struct {
    FILE* file_handle;
    int transfers;
    int status;
    bool writable;
    u32 address;
    u8 data;
} nds_firmware;

void nds_firm_next_cmd(nds_firmware* firmware);
void nds_firm_write(nds_firmware* firmware, u8 value);

#endif
