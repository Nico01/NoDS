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

#ifndef _ARM_GLOBAL_H_
#define _ARM_GLOBAL_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/*#ifndef NULL
#define NULL (void*)0
#endif

#define LOG_INFO 0
#define LOG_WARN 1
#define LOG_ERROR 2

// Fix memory bug
#define LOG(loglevel, ...) { int _line = __LINE__;\
    char message[512];\
    sprintf(message, __VA_ARGS__);\
    switch (loglevel)\
    {\
    case LOG_INFO: printf("[INFO] %s:%d: %s\n", __FILE__, _line, message); break;\
    case LOG_WARN: printf("[WARN] %s:%d: %s\n", __FILE__, _line, message); break;\
    case LOG_ERROR: printf("[ERROR] %s:%d: %s\n", __FILE__, _line, message); break;\
    }\
}

#define ASSERT(condition, loglevel, ...) { if (condition) LOG(loglevel, __VA_ARGS__) }

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;*/

#include "../common/types.h"
#include "../common/log.h"

#endif