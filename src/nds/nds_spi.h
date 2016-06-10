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

#ifndef _NDS_SPI_H_
#define _NDS_SPI_H_

#include "../common/types.h"

typedef enum {
    BAUD_4MHZ = 0,
    BAUD_2MHZ = 1,
    BAUD_1MHZ = 2,
    BAUD_512KHZ = 3
} nds_baud_rate;

typedef enum {
    SPI_POWERMAN = 0,
    SPI_FIRMWARE = 1,
    SPI_TOUCHSCR = 2,
    SPI_RESERVED = 3
} nds_spi_device;

typedef struct {
    // SPICNT
    nds_spi_device device;
    nds_baud_rate baud_rate;
    bool busy;
    bool bugged;
    bool ireq;
    bool enable;
    bool chipselect[3];


} nds_spi_bus;

u8 nds_spi_read(nds_spi_bus* spi_bus);
void nds_spi_write(nds_spi_bus* spi_bus, u8 value);

#endif
