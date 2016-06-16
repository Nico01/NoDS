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

#include "nds_spi.h"
#include "../common/log.h"

u8 nds_spi_read(nds_spi_bus* spi_bus)
{
    if (!spi_bus->enable) {
        LOG(LOG_ERROR, "SPI: read even though not enabled");
        return;
    }

    switch (spi_bus->device) {
    case SPI_POWERMAN:
        LOG(LOG_INFO, "SPI: read from POWERMANAGER");
        break;
    case SPI_FIRMWARE:
        LOG(LOG_INFO, "SPI: read from FIRMWARE (%x)", spi_bus->firmware.data);
        return spi_bus->firmware.data;
    case SPI_TOUCHSCR:
        LOG(LOG_INFO, "SPI: read from TOUCHSCREEN");
        break;
    case SPI_RESERVED:
        LOG(LOG_INFO, "SPI: read from RESERVED (odd code?)");
    }
    return 0;
}

void nds_spi_write(nds_spi_bus* spi_bus, u8 value)
{
    if (!spi_bus->enable) {
        LOG(LOG_ERROR, "SPI: write even though not enabled");
        return;
    }

    switch (spi_bus->device) {
    case SPI_POWERMAN:
        LOG(LOG_INFO, "SPI: write to POWERMANAGER (%x)", value);
        break;
    case SPI_FIRMWARE:
        LOG(LOG_INFO, "SPI: write to FIRMWARE (%x)", value);
        nds_firm_write(&spi_bus->firmware, value, spi_bus->cs_hold);
        break;
    case SPI_TOUCHSCR:
        LOG(LOG_INFO, "SPI: write to TOUCHSCREEN (%x)", value);
        break;
    case SPI_RESERVED:
        LOG(LOG_INFO, "SPI: write to RESERVED (odd code?) (%x)", value);
    }
}
