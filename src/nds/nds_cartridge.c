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

#include "../common/log.h"
#include "nds_cartridge.h"

void nds_cart_get_type(nds_cartridge* cart)
{
    // We assume for now that ROM is not encrypted.
    cart->type = CART_DUMPED;
}

nds_cartridge* nds_cart_open(char* rom_path)
{
    // TODO: sanitize cartridge header, decrypt
    nds_cartridge* cart = malloc(sizeof(nds_cartridge));
    FILE* rom_handle = fopen(rom_path, "rb");

    if (rom_handle == NULL) {
        LOG(LOG_ERROR, "fopen: %s", rom_path);
        free(cart);
        return NULL;
    }
    
    if (fread(&cart->header, sizeof(nds_header), 1, rom_handle) != 1) {
        LOG(LOG_ERROR, "fread: %s", rom_path);
        fclose(rom_handle);
        free(cart);
        return NULL;
    }
    
    cart->rom_handle = rom_handle;
    nds_cart_get_type(cart);
    
    return cart;
}

void nds_cart_close(nds_cartridge* cart)
{
    fclose(cart->rom_handle);
    free(cart);
}