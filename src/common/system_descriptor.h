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

#ifndef _SYSTEM_DESCRIPTOR_H_
#define _SYSTEM_DESCRIPTOR_H_

typedef void* (*open_func)(char* rom_path);
typedef void (*close_func)(void* system);
typedef void (*frame_func)(void* system);

typedef struct {
    char* name;
    int screen_width;
    int screen_height;
    open_func open;
    close_func close;
    frame_func frame;
} system_descriptor;

#endif
