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

#include "arm_global.h"
#include "arm_cpu.h"
#include "arm_macro.h"
#include "arm_decode.h"

void arm4_execute(arm_cpu* cpu, u32 instruction)
{
    arm_state* state = cpu->state;
    arm_instruction type = arm_decode(instruction);

    // Return if the instruction condition is not met
    CONDITION_BREAK(instruction >> 28);

    // Perform the actual execution
    switch (type) {
    case ARM_1: {
        // ARM.1 Multiply (accumulate)
        int reg_operand1 = instruction & 0xF;
        int reg_operand2 = (instruction >> 8) & 0xF;
        int reg_operand3 = (instruction >> 12) & 0xF;
        int reg_dest = (instruction >> 16) & 0xF;
        bool set_flags = instruction & (1 << 20);
        bool accumulate = instruction & (1 << 21);

        // Multiply rOP1 with rOP2, store result in rDST
        REG(reg_dest) = REG(reg_operand1) * REG(reg_operand2);

        // When the accumulate bit is set the value of an
        // additional register will be added to the result
        if (accumulate) {
            REG(reg_dest) += REG(reg_operand3);
        }

        // When the S bit is set the zero and sign flags
        // must be update according to the result
        if (set_flags) {
            CALC_SIGN(REG(reg_dest));
            CALC_ZERO(REG(reg_dest));
        }
        return;
    }
    case ARM_2: {
        // ARM.2 Multiply (accumulate) long
        int reg_operand1 = instruction & 0xF;
        int reg_operand2 = (instruction >> 8) & 0xF;
        int reg_dest_low = (instruction >> 12) & 0xF;
        int reg_dest_high = (instruction >> 16) & 0xF;
        bool set_flags = instruction & (1 << 20);
        bool accumulate = instruction & (1 << 21);
        bool sign_extend = instruction & (1 << 22);
        s64 result;

        // Since this is a *64 bit* addition but ARM registers
        // are only are *32 bit* wide the operands can be
        // sign-extended in order to preserve the sign bit
        if (sign_extend) {
            s64 operand1 = REG(reg_operand1);
            s64 operand2 = REG(reg_operand2);

            // If bit 31 is set we simply force bits 32-63 to high
            operand1 |= operand1 & 0x80000000 ? 0xFFFFFFFF00000000 : 0;
            operand2 |= operand2 & 0x80000000 ? 0xFFFFFFFF00000000 : 0;

            // Perform the multiplication and store result
            result = operand1 * operand2;
        } else {
            result = (u64)REG(reg_operand1) * (u64)REG(reg_operand2);
        }

        // When the accumulate bit is set the value of an
        // a long consisting of the two destination registers
        // will be added to the result
        if (accumulate) {
            s64 value;

            // Basically construct (rHIGH << 32) | rLOW
            value = REG(reg_dest_high);
            value <<= 16;
            value <<= 16;
            value |= REG(reg_dest_low);

            // Add the generated long
            result += value;
        }

        // Split the result into the two destination registers
        REG(reg_dest_low) = result & 0xFFFFFFFF;
        REG(reg_dest_high) = result >> 32;

        // When the S bit is set the zero and sign flags
        // must be update according to the result
        if (set_flags) {
            CALC_SIGN(REG(reg_dest_high));
            CALC_ZERO(result);
        }
        return;
    }
    case ARM_3: {
        // ARM.3 Branch and exchange
        int reg_address = instruction & 0xF;

        // When the LSB of the address is set this indicates
        // a switch into THUMB execution mode. This involves
        // setting the THUMB bit in the program status register
        if (REG(reg_address) & 1) {
            state->r15 = REG(reg_address) & ~1;
            state->cpsr |= CPSR_THUMB;
        } else {
            state->r15 = REG(reg_address) & ~3;
        }

        // Flush the CPU pipeline in order to
        // fetch instructions from the new PC
        cpu->pipeline.flush = true;

        return;
    }
    case ARM_4: {
        // ARM.4 Single data swap
        int reg_source = instruction & 0xF;
        int reg_dest = (instruction >> 12) & 0xF;
        int reg_base = (instruction >> 16) & 0xF;
        bool swap_byte = instruction & (1 << 22);
        u32 memory_value;

        // Single Data Swap instructions may not use r15
        ASSERT(reg_source == 15, LOG_ERROR, "ARM.4 rSRC=15, r15=%x", state->r15);
        ASSERT(reg_dest == 15, LOG_ERROR, "ARM.4 rDST=15, r15=%x", state->r15);
        ASSERT(reg_base == 15, LOG_ERROR, "ARM.4 rBSE=15, r15=%x", state->r15);

        // If the swap bit is set the byte at *rBSE
        // get overwritten with the LSB of rSRC and
        // the *old* value at *rBSE will be storen in rDST
        if (swap_byte) {
            memory_value = MEM_READ_8(REG(reg_base));
            MEM_WRITE_8(REG(reg_base), REG(reg_source));
            REG(reg_dest) = memory_value;
        } else {
            u32 address = REG(reg_base);
            int amount = (address & 3) * 8;

            // Emulate rotated read
            memory_value = MEM_READ_32(address);
            if (amount != 0) {
                memory_value = (memory_value >> amount) | (memory_value << (32 - amount));
            }

            MEM_WRITE_32(address, REG(reg_source));
            REG(reg_dest) = memory_value;
        }
        return;
    }
    // TODO: Recheck for correctness and look for possabilities to optimize this bunch of code
    case ARM_5:
    case ARM_6:
    case ARM_7: {
        // ARM.5 Halfword data transfer, register offset
        // ARM.6 Halfword data transfer, immediate offset
        // ARM.7 Signed data transfer (byte/halfword)
        u32 offset;
        int reg_dest = (instruction >> 12) & 0xF;
        int reg_base = (instruction >> 16) & 0xF;
        bool load = instruction & (1 << 20);
        bool write_back = instruction & (1 << 21);
        bool immediate = instruction & (1 << 22);
        bool add_to_base = instruction & (1 << 23);
        bool pre_indexed = instruction & (1 << 24);
        u32 address = REG(reg_base);

        // Instructions neither write back if base register is r15 nor should they have the write-back bit set when being post-indexed (post-indexing automatically writes back the address)
        ASSERT(reg_base == 15 && write_back, LOG_WARN, "Halfword and Signed Data Transfer, thou shall not writeback to r15, r15=0x%x", state->r15);
        ASSERT(write_back && !pre_indexed, LOG_WARN, "Halfword and Signed Data Transfer, thou shall not have write-back bit if being post-indexed, r15=0x%x", state->r15);

        // Load-bit shall not be unset when signed transfer is selected
        ASSERT(type == ARM_7 && !load, LOG_WARN, "Halfword and Signed Data Transfer, thou shall not store in signed mode, r15=0x%x", state->r15);

        // If the instruction is immediate take an 8-bit immediate value as offset, otherwise take the contents of a register as offset
        if (immediate) {
            offset = (instruction & 0xF) | ((instruction >> 4) & 0xF0);
        } else {
            int reg_offset = instruction & 0xF;

            ASSERT(reg_offset == 15, LOG_WARN, "Halfword and Signed Data Transfer, thou shall not take r15 as offset, r15=0x%x", state->r15);

            offset = REG(reg_offset);
        }

        // If the instruction is pre-indexed we must add/subtract the offset beforehand
        if (pre_indexed) {
            if (add_to_base) {
                address += offset;
            } else {
                address -= offset;
            }
        }

        // Perform the actual load / store operation
        if (load) {
            // TODO: Check if pipeline is flushed when reg_dest is r15
            if (type == ARM_7) {
                bool halfword = instruction & (1 << 5);
                u32 value;
                if (halfword) {
                    if (address & 1) {
                        value = MEM_READ_8(address & ~1);
                        if (value & 0x80) {
                            value |= 0xFFFFFF00;
                        }
                    } else {
                        value = MEM_READ_16(address);
                        if (value & 0x8000) {
                            value |= 0xFFFF0000;
                        }
                    }
                } else {
                    value = MEM_READ_8(address);
                    if (value & 0x80) {
                        value |= 0xFFFFFF00;
                    }
                }
                REG(reg_dest) = value;
            } else {
                REG(reg_dest) = MEM_READ_16(address);
            }
        } else {
            if (reg_dest == 15) {
                MEM_WRITE_16(address, state->r15 + 4);
            } else {
                MEM_WRITE_16(address, REG(reg_dest));
            }
        }

        // When the instruction either is pre-indexed and has the write-back bit or it's post-indexed we must writeback the calculated address
        if ((write_back || !pre_indexed) && reg_base != reg_dest) {
            if (!pre_indexed) {
                if (add_to_base) {
                    address += offset;
                } else {
                    address -= offset;
                }
            }
            REG(reg_base) = address;
        }
        return;
    }
    case ARM_8: {
        // ARM.8 Data processing and PSR transfer
        bool set_flags = instruction & (1 << 20);
        int opcode = (instruction >> 21) & 0xF;

        // Determine wether the instruction is data processing or psr transfer
        if (!set_flags && opcode >= 0b1000 && opcode <= 0b1011) {
            // PSR transfer
            bool immediate = instruction & (1 << 25);
            bool use_spsr = instruction & (1 << 22);
            bool msr = instruction & (1 << 21);

            if (msr) {
                // MSR
                u32 mask = 0;
                u32 operand;

                // Depending of the fsxc bits some bits are overwritten or not
                if (instruction & (1 << 16)) mask |= 0x000000FF;
                if (instruction & (1 << 17)) mask |= 0x0000FF00;
                if (instruction & (1 << 18)) mask |= 0x00FF0000;
                if (instruction & (1 << 19)) mask |= 0xFF000000;

                // Decode the value written to cpsr/spsr
                if (immediate) {
                    int imm = instruction & 0xFF;
                    int ror = ((instruction >> 8) & 0xF) << 1;
                    operand = (imm >> ror) | (imm << (32 - ror));
                } else {
                    int reg_source = instruction & 0xF;
                    operand = REG(reg_source);
                }

                // Write
                if (use_spsr) {
                    *state->spsr_ptr = (*state->spsr_ptr & ~mask) | (operand & mask);
                } else {
                    state->cpsr = (state->cpsr & ~mask) | (operand & mask);
                    ARM_REMAP(state);
                }
            } else {
                // MRS
                int reg_dest = (instruction >> 12) & 0xF;
                REG(reg_dest) = use_spsr ? *state->spsr_ptr : state->cpsr;
            }
        } else {
            // Data processing
            int reg_dest = (instruction >> 12) & 0xF;
            int reg_operand1 = (instruction >> 16) & 0xF;
            bool immediate = instruction & (1 << 25);
            u32 operand1 = REG(reg_operand1);
            u32 operand2;
            bool carry = state->cpsr & CPSR_CARRY; // == CPSR_CARRY;

            // Operand 2 can either be an 8 bit immediate value rotated right by 4 bit value or the value of a register shifted by a specific amount
            if (immediate) {
                int immediate_value = instruction & 0xFF;
                int amount = ((instruction >> 8) & 0xF) << 1;
                operand2 = (immediate_value >> amount) | (immediate_value << (32 - amount));
                if (amount != 0) {
                    carry = (immediate_value >> (amount - 1)) & 1;
                }
            } else {
                bool shift_immediate = (instruction & (1 << 4)) ? false : true;
                int reg_operand2 = instruction & 0xF;
                u32 amount;
                operand2 = REG(reg_operand2);

                // The amount is either the value of a register or a 5 bit immediate
                if (shift_immediate) {
                    amount = (instruction >> 7) & 0x1F;
                } else {
                    int reg_shift = (instruction >> 8) & 0xF;
                    amount = REG(reg_shift);

                    // When using a register to specify the shift amount r15 will be 12 bytes ahead instead of 8 bytes
                    if (reg_operand1 == 15) {
                        operand1 += 4;
                    }
                    if (reg_operand2 == 15) {
                        operand2 += 4;
                    }
                }

                // Perform the actual shift/rotate
                switch ((instruction >> 5) & 3) {
                case 0b00:
                    // Logical Shift Left
                    LSL(operand2, amount, carry);
                    break;
                case 0b01:
                    // Logical Shift Right
                    LSR(operand2, amount, carry, shift_immediate);
                    break;
                case 0b10: {
                    // Arithmetic Shift Right
                    ASR(operand2, amount, carry, shift_immediate);
                    break;
                }
                case 0b11:
                    // Rotate Right
                    ROR(operand2, amount, carry, shift_immediate);
                    break;
                }
            }

            // When destination register is r15 and s bit is set rather than updating the flags restore cpsr
            // This is allows for restoring r15 and cpsr at the same time
            if (reg_dest == 15 && set_flags) {
                set_flags = false;
                state->cpsr = *state->spsr_ptr;
                ARM_REMAP(state);
            }

            // Perform the actual operation
            switch (opcode) {
            case 0b0000: { // AND
                u32 result = operand1 & operand2;
                if (set_flags) {
                    CALC_SIGN(result);
                    CALC_ZERO(result);
                    SET_CARRY(carry);
                }
                REG(reg_dest) = result;
                break;
            }
            case 0b0001: { // EOR
                u32 result = operand1 ^ operand2;
                if (set_flags) {
                    CALC_SIGN(result);
                    CALC_ZERO(result);
                    SET_CARRY(carry);
                }
                REG(reg_dest) = result;
                break;
            }
            case 0b0010: { // SUB
                u32 result = operand1 - operand2;
                if (set_flags) {
                    SET_CARRY(operand1 >= operand2);
                    CALC_OVERFLOW_SUB(result, operand1, operand2);
                    CALC_SIGN(result);
                    CALC_ZERO(result);
                }
                REG(reg_dest) = result;
                break;
            }
            case 0b0011: { // RSB
                u32 result = operand2 - operand1;
                if (set_flags) {
                    SET_CARRY(operand2 >= operand1);
                    CALC_OVERFLOW_SUB(result, operand2, operand1);
                    CALC_SIGN(result);
                    CALC_ZERO(result);
                }
                REG(reg_dest) = result;
                break;
            }
            case 0b0100: { // ADD
                u32 result = operand1 + operand2;
                if (set_flags) {
                    u64 result_long = (u64)operand1 + (u64)operand2;
                    SET_CARRY(result_long & 0x100000000);
                    CALC_OVERFLOW_ADD(result, operand1, operand2);
                    CALC_SIGN(result);
                    CALC_ZERO(result);
                }
                REG(reg_dest) = result;
                break;
            }
            case 0b0101: { // ADC
                int carry2 = (state->cpsr >> 29) & 1;
                u32 result = operand1 + operand2 + carry2;
                if (set_flags) {
                    u64 result_long = (u64)operand1 + (u64)operand2 + (u64)carry2;
                    SET_CARRY(result_long & 0x100000000);
                    CALC_OVERFLOW_ADD(result, operand1, operand2 + carry2);
                    CALC_SIGN(result);
                    CALC_ZERO(result);
                }
                REG(reg_dest) = result;
                break;
            }
            case 0b0110: { // SBC
                int carry2 = (state->cpsr >> 29) & 1;
                u32 result = operand1 - operand2 + carry2 - 1;
                if (set_flags) {
                    SET_CARRY(operand1 >= (operand2 + carry2 - 1));
                    CALC_OVERFLOW_SUB(result, operand1, (operand2 + carry2 - 1));
                    CALC_SIGN(result);
                    CALC_ZERO(result);
                }
                REG(reg_dest) = result;
                break;
            }
            case 0b0111: { // RSC
                int carry2 = (state->cpsr >> 29) & 1;
                u32 result = operand2 - operand1 + carry2 - 1;
                if (set_flags) {
                    SET_CARRY(operand2 >= (operand1 + carry2 - 1));
                    CALC_OVERFLOW_SUB(result, operand2, (operand1 + carry2 - 1));
                    CALC_SIGN(result);
                    CALC_ZERO(result);
                }
                REG(reg_dest) = result;
                break;
            }
            case 0b1000: { // TST
                u32 result = operand1 & operand2;
                CALC_SIGN(result);
                CALC_ZERO(result);
                SET_CARRY(carry);
                break;
            }
            case 0b1001: { // TEQ
                u32 result = operand1 ^ operand2;
                CALC_SIGN(result);
                CALC_ZERO(result);
                SET_CARRY(carry);
                break;
            }
            case 0b1010: { // CMP
                u32 result = operand1 - operand2;
                SET_CARRY(operand1 >= operand2);
                CALC_OVERFLOW_SUB(result, operand1, operand2);
                CALC_SIGN(result);
                CALC_ZERO(result);
                break;
            }
            case 0b1011: { // CMN
                u32 result = operand1 + operand2;
                u64 result_long = (u64)operand1 + (u64)operand2;
                SET_CARRY(result_long & 0x100000000);
                CALC_OVERFLOW_ADD(result, operand1, operand2);
                CALC_SIGN(result);
                CALC_ZERO(result);
                break;
            }
            case 0b1100: { // ORR
                u32 result = operand1 | operand2;
                if (set_flags) {
                    CALC_SIGN(result);
                    CALC_ZERO(result);
                    SET_CARRY(carry);
                }
                REG(reg_dest) = result;
                break;
            }
            case 0b1101: { // MOV
                if (set_flags) {
                    CALC_SIGN(operand2);
                    CALC_ZERO(operand2);
                    SET_CARRY(carry);
                }
                REG(reg_dest) = operand2;
                break;
            }
            case 0b1110: { // BIC
                u32 result = operand1 & ~operand2;
                if (set_flags) {
                    CALC_SIGN(result);
                    CALC_ZERO(result);
                    SET_CARRY(carry);
                }
                REG(reg_dest) = result;
                break;
            }
            case 0b1111: { // MVN
                u32 not_operand2 = ~operand2;
                if (set_flags) {
                    CALC_SIGN(not_operand2);
                    CALC_ZERO(not_operand2);
                    SET_CARRY(carry);
                }
                REG(reg_dest) = not_operand2;
                break;
            }
            }

            // When writing to r15 initiate pipeline flush
            if (reg_dest == 15) {
                cpu->pipeline.flush = true;
            }
        }
        return;
    }
    case ARM_9: {
        // ARM.9 Load/store register/unsigned byte (Single Data Transfer)
        // TODO: Force user mode when instruction is post-indexed and has writeback bit (in system mode only?)
        u32 offset;
        int reg_dest = (instruction >> 12) & 0xF;
        int reg_base = (instruction >> 16) & 0xF;
        bool load = instruction & (1 << 20);
        bool write_back = instruction & (1 << 21);
        bool transfer_byte = instruction & (1 << 22);
        bool add_to_base = instruction & (1 << 23);
        bool pre_indexed = instruction & (1 << 24);
        bool immediate = (instruction & (1 << 25)) == 0;
        u32 address = REG(reg_base);

        // Instructions neither write back if base register is r15 nor should they have the write-back bit set when being post-indexed (post-indexing automatically writes back the address)
        ASSERT(reg_base == 15 && write_back, LOG_WARN, "Single Data Transfer, thou shall not writeback to r15, r15=0x%x", state->r15);
        ASSERT(write_back && !pre_indexed, LOG_WARN, "Single Data Transfer, thou shall not have write-back bit if being post-indexed, r15=0x%x", state->r15);

        // The offset added to the base address can either be an 12 bit immediate value or a register shifted by 5 bit immediate value
        if (immediate) {
            offset = instruction & 0xFFF;
        } else {
            int reg_offset = instruction & 0xF;
            u32 amount = (instruction >> 7) & 0x1F;
            int shift = (instruction >> 5) & 3;
            bool carry;

            ASSERT(reg_offset == 15, LOG_WARN, "Single Data Transfer, thou shall not use r15 as offset, r15=0x%x", state->r15);

            offset = REG(reg_offset);

            // Perform the actual shift
            switch (shift) {
            case 0b00: {
                // Logical Shift Left
                LSL(offset, amount, carry);
                break;
            }
            case 0b01: {
                // Logical Shift Right
                LSR(offset, amount, carry, true);
                break;
            }
            case 0b10: {
                // Arithmetic Shift Right
                ASR(offset, amount, carry, true);
                break;
            }
            case 0b11: {
                // Rotate Right
                carry = state->cpsr & CPSR_CARRY;
                ROR(offset, amount, carry, true);
                break;
            }
            }
        }

        // If the instruction is pre-indexed we must add/subtract the offset beforehand
        if (pre_indexed) {
            if (add_to_base) {
                address += offset;
            } else {
                address -= offset;
            }
        }

        // Perform the actual load / store operation
        if (load) {
            if (transfer_byte) {
                REG(reg_dest) = MEM_READ_8(address);
            } else {
                u32 word = MEM_READ_32(address & ~3);
                int amount = (address & 3) * 8;
                if (amount != 0) {
                    word = (word >> amount) | (word << (32 - amount));
                }
                REG(reg_dest) = word;
            } if (reg_dest == 15) {
                cpu->pipeline.flush = true;
            }
        } else {
            u32 value = REG(reg_dest);
            if (reg_dest == 15) {
                value += 4;
            }
            if (transfer_byte) {
                MEM_WRITE_8(address, value & 0xFF);
            } else {
                MEM_WRITE_32(address, value);
            }
        }

        // When the instruction either is pre-indexed and has the write-back bit or it's post-indexed we must writeback the calculated address
        if (reg_base != reg_dest) {
            if (!pre_indexed) {
                if (add_to_base) {
                    REG(reg_base) += offset;
                } else {
                    REG(reg_base) -= offset;
                }
            } else if (write_back) {
                REG(reg_base) = address;
            }
        }
        return;
    }
    case ARM_10:
        // ARM.10 Undefined
        LOG(LOG_ERROR, "Undefined instruction (0x%x), r15=0x%x", instruction, state->r15);
        return;
    case ARM_11:
    {
        // ARM.11 Block Data Transfer
        // TODO: Handle empty register list
        //       Correct transfer order for stm (this is needed for some io transfers)
        //       See gbatek for both
        bool pc_in_list = instruction & (1 << 15);
        int reg_base = (instruction >> 16) & 0xF;
        bool load = instruction & (1 << 20);
        bool write_back = instruction & (1 << 21);
        bool s_bit = instruction & (1 << 22); // TODO: Give this a meaningful name
        bool add_to_base = instruction & (1 << 23);
        bool pre_indexed = instruction & (1 << 24);
        u32 address = REG(reg_base);
        u32 old_address = address;
        bool switched_mode = false;
        int old_mode;
        int first_register = 0;

        // Base register must not be r15
        ASSERT(reg_base == 15, LOG_WARN, "Block Data Tranfser, thou shall not take r15 as base register, r15=0x%x", state->r15);

        // If the s bit is set and the instruction is either a store or r15 is not in the list switch to user mode
        if (s_bit && (!load || !pc_in_list)) {
            // Writeback must not be activated in this case
            ASSERT(write_back, LOG_WARN, "Block Data Transfer, thou shall not do user bank transfer with writeback, r15=0x%x", state->r15);

            // Save current mode and enter user mode
            old_mode = state->cpsr & 0x1F;
            state->cpsr = (state->cpsr & ~CPSR_MODE) | MODE_USR;
            ARM_REMAP(state);

            // Mark that we switched to user mode
            switched_mode = true;
        }

        // Find the first register
        for (int i = 0; i < 16; i++) {
            if (instruction & (1 << i)) {
                first_register = i;
                break;
            }
        }

        // Walk through the register list
        // TODO: Start with the first register (?)
        //       Remove code redundancy
        if (add_to_base) {
            for (int i = first_register; i < 16; i++) {
                // Determine if the current register will be loaded/saved
                if (instruction & (1 << i)) {
                    // If instruction is pre-indexed we must update address beforehand
                    if (pre_indexed) {
                        address += 4;
                    }

                    // Perform the actual load / store operation
                    if (load) {
                        // Overwriting the base disables writeback
                        if (i == reg_base) {
                            write_back = false;
                        }

                        // Load the register
                        REG(i) = MEM_READ_32(address);

                        // If r15 is overwritten, the pipeline must be flushed
                        if (i == 15) {
                            // If the s bit is set a mode switch is performed
                            if (s_bit) {
                                // spsr_<mode> must not be copied to cpsr in user mode because user mode has not such a register
                                ASSERT((state->cpsr & 0x1F) == MODE_USR, LOG_ERROR, "Block Data Transfer is about to copy spsr_<mode> to cpsr, however we are in user mode, r15=0x%x", state->r15);

                                state->cpsr = *state->spsr_ptr;
                                ARM_REMAP(state);
                            }
                            cpu->pipeline.flush = true;
                        }
                    } else {
                        // When the base register is the first register in the list its original value is written
                        if (i == first_register && i == reg_base) {
                            MEM_WRITE_32(address, old_address);
                        } else {
                            MEM_WRITE_32(address, REG(i));
                        }
                    }

                    // If instruction is not pre-indexed we must update address afterwards
                    if (!pre_indexed) {
                        address += 4;
                    }

                    // If the writeback is specified the base register must be updated after each register
                    if (write_back) {
                        REG(reg_base) = address;
                    }
                }
            }
        } else {
            for (int i = 15; i >= first_register; i--) {
                // Determine if the current register will be loaded/saved
                if (instruction & (1 << i)) {
                    // If instruction is pre-indexed we must update address beforehand
                    if (pre_indexed) {
                        address -= 4;
                    }

                    // Perform the actual load / store operation
                    if (load) {
                        // Overwriting the base disables writeback
                        if (i == reg_base) {
                            write_back = false;
                        }

                        // Load the register
                        REG(i) = MEM_READ_32(address);

                        // If r15 is overwritten, the pipeline must be flushed
                        if (i == 15) {
                            // If the s bit is set a mode switch is performed
                            if (s_bit) {
                                // spsr_<mode> must not be copied to cpsr in user mode because user mode has no such a register
                                ASSERT((state->cpsr & CPSR_MODE) == MODE_USR, LOG_ERROR, "Block Data Transfer is about to copy spsr_<mode> to cpsr, however we are in user mode, r15=0x%x", state->r15);

                                state->cpsr = *state->spsr_ptr;
                                ARM_REMAP(state);
                            }
                            cpu->pipeline.flush = true;
                        }
                    } else {
                        // When the base register is the first register in the list its original value is written
                        if (i == first_register && i == reg_base) {
                            MEM_WRITE_32(address, old_address);
                        } else {
                            MEM_WRITE_32(address, REG(i));
                        }
                    }

                    // If instruction is not pre-indexed we must update address afterwards
                    if (!pre_indexed) {
                        address -= 4;
                    }

                    // If the writeback is specified the base register must be updated after each register
                    if (write_back) {
                        REG(reg_base) = address;
                    }
                }
            }
        }

        // If we switched mode it's time now to restore the previous mode
        if (switched_mode) {
            state->cpsr = (state->cpsr & ~0x1F) | old_mode;
            ARM_REMAP(state);
        }
        return;
    }
    case ARM_12: {
        // ARM.12 Branch
        bool link = instruction & (1 << 24);
        u32 offset = instruction & 0xFFFFFF;
        if (offset & 0x800000) {
            offset |= 0xFF000000;
        }
        if (link) {
            REG(14) = state->r15 - 4;
        }
        state->r15 += offset << 2;
        cpu->pipeline.flush = true;
        return;
    }
    case ARM_13:
        // ARM.13 Coprocessor data transfer
        LOG(LOG_ERROR, "Unimplemented coprocessor data transfer, r15=0x%x", state->r15);
        return;
    case ARM_14:
        // ARM.14 Coprocessor data operation
        LOG(LOG_ERROR, "Unimplemented coprocessor data operation, r15=0x%x", state->r15);
        return;
    case ARM_15:
        // ARM.15 Coprocessor register transfer
        LOG(LOG_ERROR, "Unimplemented coprocessor register transfer, r15=0x%x", state->r15);
        return;
    case ARM_16:
        // ARM.16 Software interrupt
        if (cpu->svc_handler.method == NULL) {
            state->r_svc[1] = state->r15 - SIZE_WORD;
            state->r15 = cpu->base_vector + EXCPT_SOFTWARE;
            state->spsr_svc = state->cpsr;
            state->cpsr = (state->cpsr & ~CPSR_MODE) | MODE_SVC | CPSR_IRQ_DISABLE;
            ARM_REMAP(state);
            cpu->pipeline.flush = true;
        } else {
            cpu->svc_handler.method(cpu, cpu->svc_handler.object);
        }
        return;
    }
}
