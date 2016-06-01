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

#ifndef _ARM_DECODE_H_
#define _ARM_DECODE_H_

#include "arm_global.h"

typedef enum {
    ARM_1,
    ARM_2,
    ARM_3,
    ARM_4,
    ARM_5,
    ARM_6,
    ARM_7,
    ARM_8,
    ARM_9,
    ARM_10,
    ARM_11,
    ARM_12,
    ARM_13,
    ARM_14,
    ARM_15,
    ARM_16,
    ARM_ERROR
} arm_instruction;

typedef enum {
    THUMB_1,
    THUMB_2,
    THUMB_3,
    THUMB_4,
    THUMB_5,
    THUMB_6,
    THUMB_7,
    THUMB_8,
    THUMB_9,
    THUMB_10,
    THUMB_11,
    THUMB_12,
    THUMB_13,
    THUMB_14,
    THUMB_15,
    THUMB_16,
    THUMB_17,
    THUMB_18,
    THUMB_19,
    THUMB_ERROR
} thumb_instruction;

arm_instruction arm_decode(u32 instruction);
thumb_instruction arm_decode_thumb(u16 instruction);

#endif