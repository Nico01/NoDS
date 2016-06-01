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

#include "arm_decode.h"

arm_instruction arm_decode(u32 instruction)
{
    u32 opcode = instruction & 0x0FFFFFFF;
    int section = ARM_ERROR;
    switch (opcode >> 26) {
    case 0b00:
        if (opcode & (1 << 25)) {
            // ARM.8 Data processing and PSR transfer ... immediate
            section = ARM_8;
        } else if ((opcode & 0xFF00FF0) == 0x1200F10) {
            // ARM.3 Branch and exchange
            section = ARM_3;
        } else if ((opcode & 0x10000F0) == 0x90) {
            // ARM.1 Multiply (accumulate), ARM.2 Multiply (accumulate) long
            section = opcode & (1 << 23) ? ARM_2 : ARM_1;
        } else if ((opcode & 0x10000F0) == 0x1000090) {
            // ARM.4 Single data swap
            section = ARM_4;
        } else if ((opcode & 0x4000F0) == 0xB0) {
            // ARM.5 Halfword data transfer, register offset
            section = ARM_5;
        } else if ((opcode & 0x4000F0) == 0x4000B0) {
            // ARM.6 Halfword data transfer, immediate offset
            section = ARM_6;
        } else if ((opcode & 0xD0) == 0xD0) {
            // ARM.7 Signed data transfer (byte/halfword)
            section = ARM_7;
        } else {
            // ARM.8 Data processing and PSR transfer
            section = ARM_8;
        }
        break;
    case 0b01:
        // ARM.9 Single data transfer, ARM.10 Undefined
        section = (opcode & 0x2000010) == 0x2000010 ? ARM_10 : ARM_9;
        break;
    case 0b10:
        // ARM.11 Block data transfer, ARM.12 Branch
        section = opcode & (1 << 25) ? ARM_12 : ARM_11;
        break;
    case 0b11:
        // TODO: Optimize with a switch?
        if (opcode & (1 << 25)) {
            if (opcode & (1 << 24)) {
                // ARM.16 Software interrupt
                section = ARM_16;
            } else {
                // ARM.14 Coprocessor data operation, ARM.15 Coprocessor register transfer
                section = opcode & 0x10 ? ARM_15 : ARM_14;
            }
        } else {
            // ARM.13 Coprocessor data transfer
            section = ARM_13;
        }
        break;
    }
    return section;
}

thumb_instruction arm_decode_thumb(u16 instruction)
{
    if ((instruction & 0xF800) < 0x1800) {
        // THUMB.1 Move shifted register
        return THUMB_1;
    } else if ((instruction & 0xF800) == 0x1800) {
        // THUMB.2 Add/subtract
        return THUMB_2;
    } else if ((instruction & 0xE000) == 0x2000) {
        // THUMB.3 Move/compare/add/subtract immediate
        return THUMB_3;
    } else if ((instruction & 0xFC00) == 0x4000) {
        // THUMB.4 ALU operations
        return THUMB_4;
    } else if ((instruction & 0xFC00) == 0x4400) {
        // THUMB.5 Hi register operations/branch exchange
        return THUMB_5;
    } else if ((instruction & 0xF800) == 0x4800) {
        // THUMB.6 PC-relative load
        return THUMB_6;
    } else if ((instruction & 0xF200) == 0x5000) {
        // THUMB.7 Load/store with register offset
        return THUMB_7;
    } else if ((instruction & 0xF200) == 0x5200) {
        // THUMB.8 Load/store sign-extended byte/halfword
        return THUMB_8;
    } else if ((instruction & 0xE000) == 0x6000) {
        // THUMB.9 Load store with immediate offset
        return THUMB_9;
    } else if ((instruction & 0xF000) == 0x8000) {
        // THUMB.10 Load/store halfword
        return THUMB_10;
    } else if ((instruction & 0xF000) == 0x9000) {
        // THUMB.11 SP-relative load/store
        return THUMB_11;
    } else if ((instruction & 0xF000) == 0xA000) {
        // THUMB.12 Load address
        return THUMB_12;
    } else if ((instruction & 0xFF00) == 0xB000) {
        // THUMB.13 Add offset to stack pointer
        return THUMB_13;
    } else if ((instruction & 0xF600) == 0xB400) {
        // THUMB.14 push/pop registers
        return THUMB_14;
    } else if ((instruction & 0xF000) == 0xC000) {
        // THUMB.15 Multiple load/store
        return THUMB_15;
    } else if ((instruction & 0xFF00) < 0xDF00) {
        // THUMB.16 Conditional Branch
        return THUMB_16;
    } else if ((instruction & 0xFF00) == 0xDF00) {
        // THUMB.17 Software Interrupt
        return THUMB_17;
    } else if ((instruction & 0xF800) == 0xE000) {
        // THUMB.18 Unconditional Branch
        return THUMB_18;
    } else if ((instruction & 0xF000) == 0xF000) {
        // THUMB.19 Long branch with link
        return THUMB_19;
    }
    return THUMB_ERROR;
}