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

#ifdef GDB_SUPPORT

#ifndef _GDB_H_
#define _GDB_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "../arm_cpu.h"

typedef struct {
    int socket;
    int socket2;
    struct sockaddr_in server;
    struct sockaddr_in client;
    arm_cpu* arm;
} arm_gdb;

arm_gdb* arm_gdb_make(arm_cpu* arm, int port);

#endif

#endif
