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

void arm4_execute_thumb(arm_cpu* cpu, u16 instruction)
{
    arm_state* state = cpu->state;
    switch (arm_decode_thumb(instruction)) {
    case THUMB_1: {
        // THUMB.1 Move shifted register
        int reg_dest = instruction & 7;
        int reg_source = (instruction >> 3) & 7;
        u32 immediate_value = (instruction >> 6) & 0x1F;
        int opcode = (instruction >> 11) & 3;
        bool carry = state->cpsr & CPSR_CARRY;
        
        // Sync prefetch from r15
        SYNC(state->r15, SIZE_HWORD, false, CYCLE_S);
        
        // We'll operate directly on reg_dest
        REG(reg_dest) = REG(reg_source);

        // Perform given shift
        switch (opcode) {
        case 0b00:
            LSL(REG(reg_dest), immediate_value, carry);
            SET_CARRY(carry);
            break;
        case 0b01:
            LSR(REG(reg_dest), immediate_value, carry, true);
            SET_CARRY(carry);
            break;
        case 0b10: {
            ASR(REG(reg_dest), immediate_value, carry, true);
            SET_CARRY(carry);
            break;
        }
        }

        // Update sign and zero flag
        CALC_SIGN(REG(reg_dest));
        CALC_ZERO(REG(reg_dest));
        break;
    }
    case THUMB_2: {
        // THUMB.2 Add/subtract
        int reg_dest = instruction & 7;
        int reg_source = (instruction >> 3) & 7;
        u32 operand;

        // Sync prefetch from r15
        SYNC(state->r15, SIZE_HWORD, false, CYCLE_S);
        
        // Decode third operand, either 3 bit immediate or another register
        if (instruction & (1 << 10)) {
            operand = (instruction >> 6) & 7;
        } else {
            operand = REG((instruction >> 6) & 7);
        }

        // Determine wether to subtract or add
        if (instruction & (1 << 9)) {
            u32 result = REG(reg_source) - operand;
            SET_CARRY(REG(reg_source) >= operand);
            CALC_OVERFLOW_SUB(result, REG(reg_source), operand);
            CALC_SIGN(result);
            CALC_ZERO(result);
            REG(reg_dest) = result;
        } else {
            u32 result = REG(reg_source) + operand;
            u64 result_long = (u64)(REG(reg_source)) + (u64)operand;
            SET_CARRY(result_long & 0x100000000);
            CALC_OVERFLOW_ADD(result, REG(reg_source), operand);
            CALC_SIGN(result);
            CALC_ZERO(result);
            REG(reg_dest) = result;
        }
        break;
    }
    case THUMB_3: {
        // THUMB.3 Move/compare/add/subtract immediate
        u32 immediate_value = instruction & 0xFF;
        int reg_dest = (instruction >> 8) & 7;
        
        // Sync prefetch from r15
        SYNC(state->r15, SIZE_HWORD, false, CYCLE_S);
        
        // Get operation code and perform
        switch ((instruction >> 11) & 3) {
        case 0b00: // MOV
            CALC_SIGN(0);
            CALC_ZERO(immediate_value);
            REG(reg_dest) = immediate_value;
            break;
        case 0b01: { // CMP
            u32 result = REG(reg_dest) - immediate_value;
            SET_CARRY(REG(reg_dest) >= immediate_value);
            CALC_OVERFLOW_SUB(result, REG(reg_dest), immediate_value);
            CALC_SIGN(result);
            CALC_ZERO(result);
            break;
        }
        case 0b10: { // ADD
            u32 result = REG(reg_dest) + immediate_value;
            u64 result_long = (u64)(REG(reg_dest)) + (u64)immediate_value;
            SET_CARRY(result_long & 0x100000000);
            CALC_OVERFLOW_ADD(result, REG(reg_dest), immediate_value);
            CALC_SIGN(result);
            CALC_ZERO(result);
            REG(reg_dest) = result;
            break;
        }
        case 0b11: { // SUB
            u32 result = REG(reg_dest) - immediate_value;
            SET_CARRY(REG(reg_dest) >= immediate_value);
            CALC_OVERFLOW_SUB(result, REG(reg_dest), immediate_value);
            CALC_SIGN(result);
            CALC_ZERO(result);
            REG(reg_dest) = result;
            break;
        }
        }
        break;
    }
    case THUMB_4: {
        // THUMB.4 ALU operations
        int reg_dest = instruction & 7;
        int reg_source = (instruction >> 3) & 7;
        
        // Prefretch from r15
        SYNC(state->r15, SIZE_HWORD, false, CYCLE_S);
        
        // Inctruction switch..
        switch ((instruction >> 6) & 0xF) {
        case 0b0000: // AND
            REG(reg_dest) &= REG(reg_source);
            CALC_SIGN(REG(reg_dest));
            CALC_ZERO(REG(reg_dest));
            break;
        case 0b0001: // EOR
            REG(reg_dest) ^= REG(reg_source);
            CALC_SIGN(REG(reg_dest));
            CALC_ZERO(REG(reg_dest));
            break;
        case 0b0010: { // LSL
            u32 amount = REG(reg_source);
            bool carry = state->cpsr & CPSR_CARRY;
            LSL(REG(reg_dest), amount, carry);
            SET_CARRY(carry);
            CALC_SIGN(REG(reg_dest));
            CALC_ZERO(REG(reg_dest));
            SYNC_ONE; // internal cycle
            break;
        }
        case 0b0011: { // LSR
            u32 amount = REG(reg_source);
            bool carry = state->cpsr & CPSR_CARRY;
            LSR(REG(reg_dest), amount, carry, false);
            SET_CARRY(carry);
            CALC_SIGN(REG(reg_dest));
            CALC_ZERO(REG(reg_dest));
            SYNC_ONE; // internal cycle
            break;
        }
        case 0b0100: { // ASR
            u32 amount = REG(reg_source);
            bool carry = state->cpsr & CPSR_CARRY;
            ASR(REG(reg_dest), amount, carry, false);
            SET_CARRY(carry);
            CALC_SIGN(REG(reg_dest));
            CALC_ZERO(REG(reg_dest));
            SYNC_ONE; // internal cycle
            break;
        }
        case 0b0101: { // ADC
            int carry = (state->cpsr >> 29) & 1;
            u32 result = REG(reg_dest) + REG(reg_source) + carry;
            u64 result_long = (u64)(REG(reg_dest)) + (u64)(REG(reg_source)) + (u64)carry;
            SET_CARRY(result_long & 0x100000000);
            CALC_OVERFLOW_ADD(result, REG(reg_dest), REG(reg_source) + carry);
            CALC_SIGN(result);
            CALC_ZERO(result);
            REG(reg_dest) = result;
            break;
        }
        case 0b0110: { // SBC
            int carry = (state->cpsr >> 29) & 1;
            u32 result = REG(reg_dest) - REG(reg_source) + carry - 1;
            SET_CARRY(REG(reg_dest) >= REG(reg_source) + carry - 1);
            CALC_OVERFLOW_SUB(result, REG(reg_dest), (REG(reg_source) + carry - 1));
            CALC_SIGN(result);
            CALC_ZERO(result);
            REG(reg_dest) = result;
            break;
        }
        case 0b0111: { // ROR
            u32 amount = REG(reg_source);
            bool carry = state->cpsr & CPSR_CARRY;
            ROR(REG(reg_dest), amount, carry, false);
            SET_CARRY(carry);
            CALC_SIGN(REG(reg_dest));
            CALC_ZERO(REG(reg_dest));
            SYNC_ONE; // internal cycle
            break;
        }
        case 0b1000: { // TST
            u32 result = REG(reg_dest) & REG(reg_source);
            CALC_SIGN(result);
            CALC_ZERO(result);
            break;
        }
        case 0b1001: { // NEG
            u32 result = 0 - REG(reg_source);
            SET_CARRY(0 >= REG(reg_source));
            CALC_OVERFLOW_SUB(result, 0, REG(reg_source));
            CALC_SIGN(result);
            CALC_ZERO(result);
            REG(reg_dest) = result;
            break;
        }
        case 0b1010: // CMP
        {
            u32 result = REG(reg_dest) - REG(reg_source);
            SET_CARRY(REG(reg_dest) >= REG(reg_source));
            CALC_OVERFLOW_SUB(result, REG(reg_dest), REG(reg_source));
            CALC_SIGN(result);
            CALC_ZERO(result);
            break;
        }
        case 0b1011: { // CMN
            u32 result = REG(reg_dest) + REG(reg_source);
            u64 result_long = (u64)(REG(reg_dest)) + (u64)(REG(reg_source));
            SET_CARRY(result_long & 0x100000000);
            CALC_OVERFLOW_ADD(result, REG(reg_dest), REG(reg_source));
            CALC_SIGN(result);
            CALC_ZERO(result);
            break;
        }
        case 0b1100: // ORR
            REG(reg_dest) |= REG(reg_source);
            CALC_SIGN(REG(reg_dest));
            CALC_ZERO(REG(reg_dest));
            break;
        case 0b1101: // MUL
            // todo: find out cycle calculation
            REG(reg_dest) *= REG(reg_source);
            CALC_SIGN(REG(reg_dest));
            CALC_ZERO(REG(reg_dest));
            SET_CARRY(false);
            break;
        case 0b1110: // BIC
            REG(reg_dest) &= ~(REG(reg_source));
            CALC_SIGN(REG(reg_dest));
            CALC_ZERO(REG(reg_dest));
            break;
        case 0b1111: // MVN
            REG(reg_dest) = ~(REG(reg_source));
            CALC_SIGN(REG(reg_dest));
            CALC_ZERO(REG(reg_dest));
            break;
        }
        break;
    }
    case THUMB_5: {
        // THUMB.5 Hi register operations/branch exchange
        int reg_dest = instruction & 7;
        int reg_source = (instruction >> 3) & 7;
        bool compare = false;
        u32 operand;
        
        // Both reg_dest and reg_source can encode either a low register (r0-r7) or a high register (r8-r15)
        switch ((instruction >> 6) & 3) {
        case 0b01:
            reg_source += 8;
            break;
        case 0b10:
            reg_dest += 8;
            break;
        case 0b11:
            reg_dest += 8;
            reg_source += 8;
            break;
        }

        operand = REG(reg_source);

        if (reg_source == 15) {
            operand &= ~1;
        }

        // Perform the actual operation
        switch ((instruction >> 8) & 3) {
        case 0b00:
            REG(reg_dest) += operand;
            break;
        case 0b01: {
            u32 result = REG(reg_dest) - operand;
            SET_CARRY(REG(reg_dest) >= operand);
            CALC_OVERFLOW_SUB(result, REG(reg_dest), operand);
            CALC_SIGN(result);
            CALC_ZERO(result);
            compare = true;
            break;
        }
        case 0b10: // MOV
            REG(reg_dest) = operand;
            break;
        case 0b11: // BX
            // Sync prefetch from r15 (even though result is worthless)
            SYNC(state->r15, SIZE_HWORD, false, CYCLE_N);
            
            // Switch CPU instruction set?
            if (operand & 1) {
                // Update r15
                state->r15 = operand & ~1;
                
                // Thumb pipeline refill
                SYNC(state->r15, SIZE_HWORD, false, CYCLE_S);
                SYNC(state->r15 + SIZE_HWORD, SIZE_HWORD, false, CYCLE_S);
            } else {
                // Disable thumb and update r15
                state->cpsr &= ~CPSR_THUMB;
                state->r15 = operand & ~3;
                
                // ARM pipeline refill
                SYNC(state->r15, SIZE_WORD, false, CYCLE_S);
                SYNC(state->r15 + SIZE_WORD, SIZE_WORD, false, CYCLE_S);
            }
                
            // Flush pipeline
            cpu->pipeline.flush = true;
            break;
        }

        if (reg_dest == 15 && !compare) {
            REG(reg_dest) &= ~1;
            cpu->pipeline.flush = true;
            // todo: timing
        }
        break;
    }
    case THUMB_6: {
        // THUMB.6 PC-relative load
        u32 immediate_value = instruction & 0xFF;
        int reg_dest = (instruction >> 8) & 7;
        u32 address = (state->r15 & ~2) + (immediate_value << 2);
        u32 value;
        
        // Sync prefetch from r15
        SYNC(state->r15, SIZE_HWORD, false, CYCLE_N);
        
        // Read value from address
        SYNC(address, SIZE_WORD, false, CYCLE_N);
        value = MEM_READ_32(address);
        
        // Sync next prefetch and write result
        SYNC(state->r15 + SIZE_HWORD, SIZE_HWORD, false, CYCLE_S); 
        REG(reg_dest) = value;
        break;
    }
    case THUMB_7: {
        // THUMB.7 Load/store with register offset
        int reg_dest = instruction & 7;
        int reg_base = (instruction >> 3) & 7;
        int reg_offset = (instruction >> 6) & 7;
        u32 address = REG(reg_base) + REG(reg_offset);
        
        // Sync prefetch from r15
        SYNC(state->r15, SIZE_HWORD, false, CYCLE_N);
        
        // Handle memory operation
        switch ((instruction >> 10) & 3) {
        case 0b00: // STR
            SYNC(address, SIZE_WORD, true, CYCLE_N);
            MEM_WRITE_32(address, REG(reg_dest));
            break;
        case 0b01: // STRB
            SYNC(address, SIZE_BYTE, true, CYCLE_N);
            MEM_WRITE_8(address, REG(reg_dest) & 0xFF);
            break;
        case 0b10: { // LDR
            u32 word = MEM_READ_32(address & ~3);
            int amount = (address & 3) * 8;
            
            // Sync the read
            SYNC(address, SIZE_WORD, false, CYCLE_N);
            
            // Fix unaligned value
            if (amount != 0) {
                word = (word >> amount) | (word << (32 - amount));
            }
            
            // Sync next prefetch and write result
            SYNC(state->r15 + SIZE_HWORD, SIZE_HWORD, false, CYCLE_S);
            REG(reg_dest) = word;
            break;
        }
        case 0b11: { // LDRB
            u32 value = MEM_READ_8(address);
            SYNC(address, SIZE_BYTE, false, CYCLE_N);
            SYNC(state->r15 + SIZE_HWORD, SIZE_HWORD, false, CYCLE_S);
            REG(reg_dest) = value;
            break;
        }
        }
        break;
    }
    case THUMB_8:
    {
        // THUMB.8 Load/store sign-extended byte/halfword
        int reg_dest = instruction & 7;
        int reg_base = (instruction >> 3) & 7;
        int reg_offset = (instruction >> 6) & 7;
        u32 address = REG(reg_base) + REG(reg_offset);
        
        switch ((instruction >> 10) & 3) {
        case 0b00: // STRH
            MEM_WRITE_16(address, REG(reg_dest));
            break;
        case 0b01: // LDSB
            REG(reg_dest) = MEM_READ_8(address);
            if (REG(reg_dest) & 0x80) {
                REG(reg_dest) |= 0xFFFFFF00;
            }
            break;
        case 0b10: // LDRH
            REG(reg_dest) = MEM_READ_16(address);
            break;
        case 0b11: { // LDSH
            u32 value = 0;
            if (address & 1) {
                value = MEM_READ_8(address &  ~1);
                if (value & 0x80) {
                    value |= 0xFFFFFF00;
                }
            } else {
                value = MEM_READ_16(address);
                if (value & 0x8000) {
                    value |= 0xFFFF0000;
                }
            }
            REG(reg_dest) = value;
            break;
        }
        }
        break;
    }
    case THUMB_9: {
        // THUMB.9 Load store with immediate offset
        int reg_dest = instruction & 7;
        int reg_base = (instruction >> 3) & 7;
        u32 immediate_value = (instruction >> 6) & 0x1F;
        
        switch ((instruction >> 11) & 3) {
        case 0b00: // STR
            MEM_WRITE_32(REG(reg_base) + (immediate_value << 2), REG(reg_dest));
            break;
        case 0b01: { // LDR
            u32 address = REG(reg_base) + (immediate_value << 2);
            u32 word = MEM_READ_32(address & ~3);
            int amount = (address & 3) * 8;
            if (amount != 0) {
                word = (word >> amount) | (word << (32 - amount));
            }
            REG(reg_dest) = word;
            break;
        }
        case 0b10: // STRB
            MEM_WRITE_8(REG(reg_base) + immediate_value, REG(reg_dest));
            break;
        case 0b11: // LDRB
            REG(reg_dest) = MEM_READ_8(REG(reg_base) + immediate_value);
            break;
        }
        break;
    }
    case THUMB_10: {
        // THUMB.10 Load/store halfword
        int reg_dest = instruction & 7;
        int reg_base = (instruction >> 3) & 7;
        u32 immediate_value = (instruction >> 6) & 0x1F;
        
        // LDRH / STRH
        if (instruction & (1 << 11)) {
            REG(reg_dest) = MEM_READ_16(REG(reg_base) + (immediate_value << 1));
        } else {
            MEM_WRITE_16(REG(reg_base) + (immediate_value << 1), REG(reg_dest));
        }
        break;
    }
    case THUMB_11: {
        // THUMB.11 SP-relative load/store
        u32 immediate_value = instruction & 0xFF;
        int reg_dest = (instruction >> 8) & 7;
        
        // LDR / STR
        if (instruction & (1 << 11)) {
            u32 address = REG(13) + (immediate_value << 2);
            u32 word = MEM_READ_32(address & ~3);
            int amount = (address & 3) * 8;
            if (amount != 0) {
                word = (word >> amount) | (word << (32 - amount));
            }
            REG(reg_dest) = word;
        } else {
            MEM_WRITE_32(REG(13) + (immediate_value << 2), REG(reg_dest));
        }
        break;
    }
    case THUMB_12: {
        // THUMB.12 Load address
        u32 immediate_value = instruction & 0xFF;
        int reg_dest = (instruction >> 8) & 7;
        
        // SP or PC as base
        if (instruction & (1 << 11)) {
            REG(reg_dest) = REG(13) + (immediate_value << 2);
        } else {
            REG(reg_dest) = (state->r15 & ~2) + (immediate_value << 2);
        }
        break;
    }
    case THUMB_13: {
        // THUMB.13 Add offset to stack pointer
        u32 immediate_value = (instruction & 0x7F) << 2;
        
        if (instruction & 0x80) {
            REG(13) -= immediate_value;
        } else {
            REG(13) += immediate_value;
        }
        break;
    }
    case THUMB_14: {
        // THUMB.14 push/pop registers
        if (instruction & (1 << 11)) { // POP
            // Load specified registers
            for (int i = 0; i <= 7; i++) {
                if (instruction & (1 << i)) {
                    REG(i) = MEM_READ_32(REG(13));
                    REG(13) += 4;
                }
            }
            
            // Restore state->r15 if neccessary
            if (instruction & (1 << 8)) {
                state->r15 = MEM_READ_32(REG(13)) & ~1;
                REG(13) += 4;
                cpu->pipeline.flush = true;
            }
        } else { // PUSH
            // Store r14 if neccessary
            if (instruction & (1 << 8)) {
                REG(13) -= 4;
                MEM_WRITE_32(REG(13), REG(14));
            }
            
            // Store specified registers
            for (int i = 7; i >= 0; i--) {
                if (instruction & (1 << i)) {
                    REG(13) -= 4;
                    MEM_WRITE_32(REG(13), REG(i));
                }
            }
        }
        break;
    }
    case THUMB_15: {
        // THUMB.15 Multiple load/store
        // TODO: Handle empty register list
        int reg_base = (instruction >> 8) & 7;
        bool write_back = true;
        u32 address = REG(reg_base);
        int first_register = 0;

        // Find the first register
        for (int i = 0; i < 8; i++) {
            if (instruction & (1 << i)) {
                first_register = i;
                break;
            }
        }

        // Run either LDMIA or STMIA code
        if (instruction & (1 << 11)) { // LDMIA
            for (int i = 0; i <= 7; i++) {
                if (instruction & (1 << i)) {
                    if (i == reg_base) {
                        write_back = false;
                    }
                    REG(i) = MEM_READ_32(address);
                    address += 4;
                    if (write_back) {
                        REG(reg_base) = address;
                    }
                }
            }
        } else { // STMIA
            for (int i = 0; i <= 7; i++) {
                if (instruction & (1 << i)) {
                    if (i == reg_base && i == first_register) {
                        MEM_WRITE_32(REG(reg_base), address);
                    } else {
                        MEM_WRITE_32(REG(reg_base), REG(i));
                    }
                    REG(reg_base) += 4;
                }
            }
        }
        break;
    }
    case THUMB_16: {
        // THUMB.16 Conditional branch
        // TODO: takes only 1S if condition not met
        u32 signed_immediate = instruction & 0xFF;

        // Sync prefetch from r15
        SYNC(state->r15, SIZE_HWORD, false, CYCLE_N);
        
        // Return if the instruction condition is not met
        CONDITION_BREAK((instruction >> 8) & 0xF);

        // Sign-extend the immediate value if neccessary
        if (signed_immediate & 0x80) {
            signed_immediate |= 0xFFFFFF00;
        }

        // Update r15 and flush pipeline
        state->r15 += (signed_immediate << 1);
        cpu->pipeline.flush = true;
        
        // Sync the pipeline refill
        SYNC(state->r15, SIZE_HWORD, false, CYCLE_S);
        SYNC(state->r15 + SIZE_HWORD, SIZE_HWORD, false, CYCLE_S);
        break;
    }
    case THUMB_17:
        // THUMB.17 Software Interrupt
        if (cpu->svc_handler.method == NULL) {
            state->r_svc[1] = state->r15 - SIZE_HWORD;
            state->r15 = cpu->base_vector + EXCPT_SOFTWARE;
            state->spsr_svc = state->cpsr;
            state->cpsr = (state->cpsr & ~(CPSR_MODE | CPSR_THUMB)) | MODE_SVC | CPSR_IRQ_DISABLE;
            ARM_REMAP(state);
            cpu->pipeline.flush = true;
        } else {
            cpu->svc_handler.method(cpu, cpu->svc_handler.object);
        }
        break;
    case THUMB_18: {
        // THUMB.18 Unconditional branch
        u32 immediate_value = (instruction & 0x3FF) << 1;
        
        // Sync prefetch from r15
        SYNC(state->r15, SIZE_HWORD, false, CYCLE_N);
        
        // Sign-extend the immediate value if neccessary
        if (instruction & 0x400) {
            immediate_value |= 0xFFFFF800;
        }
        
        // Update r15 and flush pipeline
        state->r15 += immediate_value;
        cpu->pipeline.flush = true;
        
        // Sync pipeline refill
        SYNC(state->r15, SIZE_HWORD, false, CYCLE_S);
        SYNC(state->r15 + SIZE_HWORD, SIZE_HWORD, false, CYCLE_S);
        break;
    }
    case THUMB_19:
    {
        // THUMB.19 Branch with link
        // TODO: timings
        u32 immediate_value = instruction & 0x7FF;
        if (instruction & (1 << 11)) { // BH
            u32 temp_pc = state->r15 - SIZE_HWORD;
            u32 value = REG(14) + (immediate_value << 1);

            // unsure about exact functionality
            value &= 0x7FFFFF;
            state->r15 &= ~0x7FFFFF;
            state->r15 |= value & ~1;

            REG(14) = temp_pc | 1;
            cpu->pipeline.flush = true;
        } else { // BL
            REG(14) = state->r15 + (immediate_value << 12);
        }
        break;
    }
    }
}
