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

#include <SDL/SDL.h>
#include "../../common/types.h"
#include "../../common/log.h"
#include "../../arm/gdb/arm_gdb.h"
#include "../../nds/nds_cartridge.h"
#include "../../nds/nds_system.h"
#include "../../version.h"

SDL_Surface* window;

void create_window(int width, int height)
{
    // Init SDL
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        LOG(LOG_ERROR, "SDL_Init: %s", SDL_GetError());
        SDL_Quit();
    }

    // Create SDL window
    window = SDL_SetVideoMode(width, height, 32, SDL_HWSURFACE);
    if (window == NULL) {
        LOG(LOG_ERROR, "SDL_SetVideoMode: %s", SDL_GetError());
        SDL_Quit();
    }

    // Set window title
    SDL_WM_SetCaption("NoDS " VERSION_STRING, "NoDS");
}

int main(int argc, char** argv)
{
    SDL_Event event;
    nds_cartridge* cart;
    nds_system* system;
    bool running = true;
    system_descriptor descriptor = nds_descriptor;

    if (argc != 2) {
        puts("usage: ./nods rom_path");
        return 0;
    }

    // Open supplied ROM.
    cart = nds_cart_open(argv[1]);
    system = nds_make(cart);

    // Did we read the file?
    if (cart == NULL) {
        LOG(LOG_ERROR, "nds_cart_open: cannot open file.");
        SDL_Quit();
    }

    // Debug output (header)
    LOG(LOG_INFO, "game_code=%s", NDS_STRING(cart->header.game_title, 12));
    LOG(LOG_INFO, "arm9_rom=%x", cart->header.arm9.rom);
    LOG(LOG_INFO, "arm9_ram=%x", cart->header.arm9.ram);
    LOG(LOG_INFO, "arm9_entry=%x", cart->header.arm9.entry);
    LOG(LOG_INFO, "arm9_size=%x", cart->header.arm9.size);
    LOG(LOG_INFO, "arm7_rom=%x", cart->header.arm7.rom);
    LOG(LOG_INFO, "arm7_ram=%x", cart->header.arm7.ram);
    LOG(LOG_INFO, "arm7_entry=%x", cart->header.arm7.entry);
    LOG(LOG_INFO, "arm7_size=%x", cart->header.arm7.size);

    // Setup window
    create_window(descriptor.screen_width, descriptor.screen_height);

    // SDL mainloop
    while (running) {
        //arm_step(system->arm7);
        nds_frame(system);

        // Process SDL events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }
        SDL_Flip(window);
    }

    SDL_FreeSurface(window);
    return 0;
}
