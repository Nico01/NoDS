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

#include "nds_firmware.h"
#include "../common/log.h"

void nds_firm_write(nds_firmware* firmware, u8 value, bool hold)
{
    if (firmware->transfers == 0) {
        firmware->command = value;
        switch (value) {
        case FIRM_CMD_WREN:
            firmware->write_enable = true;
            break;
        case FIRM_CMD_WRDI:
            firmware->write_enable = false;
            break;
        case FIRM_CMD_RDID:
            //...?
            break;
        case FIRM_CMD_RDSR:
            //...?
            break;
        case FIRM_CMD_READ:
        case FIRM_CMD_FAST:
            firmware->read_addr = true;
            break;
        default:
            LOG(LOG_ERROR, "SPI_FIRM: Unimplemented command %x", value);
            break;
        }
    } else if (firmware->transfers < 4 && firmware->read_addr) {
        if (firmware->transfers == 1) {
            firmware->address = value;
        } else {
            firmware->address = (firmware->address << 8) | value;
        }
    } else {
        switch (firmware->command) {
        case FIRM_CMD_RDID:
            switch (firmware->transfers) {
            case 1: firmware->data = 0x20; break;
            case 2: firmware->data = 0x50; break;
            case 3: firmware->data = 0x12; break;
            default:
                LOG(LOG_ERROR, "SPI_FIRM: RDID: read more than 3 bytes");
                break;
            }
            break;
        case FIRM_CMD_RDSR:
            // TODO: write/program/erase in process?
            firmware->data = firmware->write_enable ? 2 : 0;
            break;
        case FIRM_CMD_FAST:
            // ignore dummy byte
            if (firmware->transfers == 1) {
                break;
            }
            // fall through
        case FIRM_CMD_READ: {
            // TODO: implement prefetch algorithm which generally reads
            //       a block of bytes on READ so we don't have to fread always
            u8 data;
            fseek(firmware->file_handle, firmware->address++, SEEK_SET);
            fread(&data, 1, 1, firmware->file_handle);
            firmware->data = data;
            break;
        }
        }
    }

    if (!hold) {
        firmware->transfers = 0;
        return;
    }

    firmware->transfers++;
}
