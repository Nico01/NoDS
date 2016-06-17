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

// do some research on this value
#define FIRMWARE_SIZE 262144

void nds_firm_next_cmd(nds_firmware* firmware)
{
    // Reset the counter of transfers since command start
    // and tell the command processor that it should interpret
    // the next written byte as the next command.
    firmware->status = FIRM_STAT_CMD;
    firmware->transfers = 0;
}

void nds_firm_write(nds_firmware* firmware, u8 value)
{
    int status = firmware->status;

    // *generally* sets data register to zero since
    // SPIDATA eventually is zero when submitting bytes that do
    // not produce any output to SPIDATA.
    firmware->data = 0;

    // FLASH chip may be in deep sleep mode where it only
    // accepts RDP (0xAB) as command, which releases deep mode.
    if (firmware->status == FIRM_STAT_DEEP) {
        LOG(LOG_INFO, "SPI: FIRM: transfer while being deep");
        if (value == FIRM_CMD_RDP) {
            LOG(LOG_INFO, "SPI: FIRM: ok! woken up!");
            firmware->status = FIRM_STAT_IDLE;
        }
        return;
    }

    // Schedules the current state of the chip. ADDR has priority over
    // DUMMY and DUMMY has priority over READ and so on since status
    // may consist of multiple states whereby only one is scheduled at
    // the time and e.g. STAT_ADDR must be scheduled before STAT_DUMMY
    // since the address should be read before ommiting the dummy byte.
    if (status == FIRM_STAT_CMD) {
        switch (value) {
        case FIRM_CMD_WREN:
            LOG(LOG_INFO, "SPI: FIRM: now writable");
            firmware->writable = true;
            firmware->status = FIRM_STAT_IDLE;
            break;
        case FIRM_CMD_WRDI:
            LOG(LOG_INFO, "SPI: FIRM: now write-protected");
            firmware->writable = false;
            firmware->status = FIRM_STAT_IDLE;
            break;
        case FIRM_CMD_RDID:
            LOG(LOG_INFO, "SPI: FIRM: reading JEDEC");
            firmware->status = FIRM_STAT_JEDEC;
            break;
        case FIRM_CMD_READ:
            LOG(LOG_INFO, "SPI: FIRM: enabled read, waiting for address");
            firmware->status = FIRM_STAT_READ | FIRM_STAT_ADDR;
            break;
        case FIRM_CMD_FAST:
            LOG(LOG_INFO, "SPI: FIRM: enabled fast read, waiting for address");
            firmware->status = FIRM_STAT_READ | FIRM_STAT_ADDR | FIRM_STAT_DUMMY;
            break;
        case FIRM_CMD_DP:
            LOG(LOG_INFO, "SPI: FIRM: going deep :(");
            firmware->status = FIRM_STAT_DEEP;
            break;
        default:
            LOG(LOG_WARN, "SPI: FIRM: unsupported or bad command 0x%x", value);
            break;
        }
    } else if (status & FIRM_STAT_ADDR) {
        switch (firmware->transfers) {
        case 1:
            firmware->address = value;
            break;
        case 2:
            firmware->address = (firmware->address << 8) | value;
            break;
        case 3:
            firmware->address = (firmware->address << 8) | value;
            firmware->status &= ~FIRM_STAT_ADDR;
            LOG(LOG_INFO, "SPI: FIRM: address transfered (0x%x)", firmware->address);
            break;
        default:
            LOG(LOG_ERROR, "SPI: FIRM: STAT_ADDR exceeds 3 transfers");
            break;
        }
    } else if (status & FIRM_STAT_DUMMY) {
        status &= ~FIRM_STAT_DUMMY;
        LOG(LOG_INFO, "SPI: FIRM: dummy byte (0x%x) ommited", value);
    } else if (status == FIRM_STAT_READ) {
        u32 address = firmware->address;

        fseek(firmware->file_handle, address, SEEK_SET);
        fread(&firmware->data, 1, 1, firmware->file_handle);
        firmware->address = (address + 1) % FIRMWARE_SIZE; // does this wrap over?

        LOG(LOG_INFO, "SPI: FIRM: read byte from 0x%x (=0x%x)", address, firmware->data);
    } else if (status == FIRM_STAT_JEDEC) {
        switch (firmware->transfers) {
        case 1:
            firmware->data = 0x20;
            LOG(LOG_INFO, "SPI: FIRM: started JEDEC transfer...");
            break;
        case 2: firmware->data = 0x50; break;
        case 3:
            firmware->data = 0x12;
            firmware->status = FIRM_STAT_IDLE;
            LOG(LOG_INFO, "SPI: FIRM: finished JEDEC transfer");
            break;
        }
    } else if (status == FIRM_STAT_STATUS) {
        firmware->data = firmware->writable ? 2 : 0;
        LOG(LOG_INFO, "SPI: FIRM: read status register (0x%x)", firmware->data);
    } else if (status == FIRM_STAT_IDLE) {
        LOG(LOG_WARN, "SPI: FIRM: transfer even though idling");
    }

    firmware->transfers++;
}
