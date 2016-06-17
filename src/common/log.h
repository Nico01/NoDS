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

#ifndef _LOG_H_
#define _LOG_H_

#include <stdio.h>

#define LOG_INFO 0
#define LOG_WARN 1
#define LOG_ERROR 2

#define RED     "\x1b[31m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define RESET   "\x1b[0m"

#ifdef DEBUG
#define LOG(loglevel, ...) { int _line = __LINE__;\
    char message[512];\
    snprintf(message, 512, __VA_ARGS__);\
    switch (loglevel)\
    {\
    case LOG_INFO: printf("[" BLUE "INFO" RESET "] %s:%d: %s\n", __FILE__, _line, message); break;\
    case LOG_WARN: printf("[" MAGENTA "INFO" RESET "] %s:%d: %s\n", __FILE__, _line, message); break;\
    case LOG_ERROR: printf("[" RED "INFO" RESET "] %s:%d: %s\n", __FILE__, _line, message); break;\
    }\
}
#else
#define LOG(loglevel, ...) {}
#endif

#ifdef DEBUG
#define ASSERT(condition, loglevel, ...) { if (condition) LOG(loglevel, __VA_ARGS__) }
#else
#define ASSERT(condition, loglevel, ...) {}
#endif

#endif
