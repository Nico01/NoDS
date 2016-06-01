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

#ifndef _ARM_MACRO_H_
#define _ARM_MACRO_H_

#include "arm_global.h"

#define MEM_READ_8(address) cpu->memory.read_byte(cpu->memory.object, address)
#define MEM_READ_16(address) cpu->memory.read_hword(cpu->memory.object, address)
#define MEM_READ_32(address) cpu->memory.read_word(cpu->memory.object, address)
#define MEM_WRITE_8(address, value) cpu->memory.write_byte(cpu->memory.object, address, value)
#define MEM_WRITE_16(address, value) cpu->memory.write_hword(cpu->memory.object, address, value)
#define MEM_WRITE_32(address, value) cpu->memory.write_word(cpu->memory.object, address, value)

#define FLUSH cpu->pipeline.status = 0;\
              cpu->pipeline.flush = false;

#define REG(i) *(state->r_ptr[(i)])

#define ARM_REMAP(state) {\
    switch (state->cpsr & CPSR_MODE) {\
    case MODE_USR:\
    case MODE_SYS:\
        state->r_ptr[8] = &state->r[8];\
        state->r_ptr[9] = &state->r[9];\
        state->r_ptr[10] = &state->r[10];\
        state->r_ptr[11] = &state->r[11];\
        state->r_ptr[12] = &state->r[12];\
        state->r_ptr[13] = &state->r[13];\
        state->r_ptr[14] = &state->r[14];\
        state->spsr_ptr = &state->spsr_safe;\
        break;\
    case MODE_FIQ:\
        state->r_ptr[8] = &state->r_fiq[0];\
        state->r_ptr[9] = &state->r_fiq[1];\
        state->r_ptr[10] = &state->r_fiq[2];\
        state->r_ptr[11] = &state->r_fiq[3];\
        state->r_ptr[12] = &state->r_fiq[4];\
        state->r_ptr[13] = &state->r_fiq[5];\
        state->r_ptr[14] = &state->r_fiq[6];\
        state->spsr_ptr = &state->spsr_fiq;\
        break;\
    case MODE_IRQ:\
        state->r_ptr[8] = &state->r[8];\
        state->r_ptr[9] = &state->r[9];\
        state->r_ptr[10] = &state->r[10];\
        state->r_ptr[11] = &state->r[11];\
        state->r_ptr[12] = &state->r[12];\
        state->r_ptr[13] = &state->r_irq[0];\
        state->r_ptr[14] = &state->r_irq[1];\
        state->spsr_ptr = &state->spsr_irq;\
        break;\
    case MODE_SVC:\
        state->r_ptr[8] = &state->r[8];\
        state->r_ptr[9] = &state->r[9];\
        state->r_ptr[10] = &state->r[10];\
        state->r_ptr[11] = &state->r[11];\
        state->r_ptr[12] = &state->r[12];\
        state->r_ptr[13] = &state->r_svc[0];\
        state->r_ptr[14] = &state->r_svc[1];\
        state->spsr_ptr = &state->spsr_svc;\
        break;\
    case MODE_ABT:\
        state->r_ptr[8] = &state->r[8];\
        state->r_ptr[9] = &state->r[9];\
        state->r_ptr[10] = &state->r[10];\
        state->r_ptr[11] = &state->r[11];\
        state->r_ptr[12] = &state->r[12];\
        state->r_ptr[13] = &state->r_abt[0];\
        state->r_ptr[14] = &state->r_abt[1];\
        state->spsr_ptr = &state->spsr_abt;\
        break;\
    case MODE_UND:\
        state->r_ptr[8] = &state->r[8];\
        state->r_ptr[9] = &state->r[9];\
        state->r_ptr[10] = &state->r[10];\
        state->r_ptr[11] = &state->r[11];\
        state->r_ptr[12] = &state->r[12];\
        state->r_ptr[13] = &state->r_und[0];\
        state->r_ptr[14] = &state->r_und[1];\
        state->spsr_ptr = &state->spsr_und;\
        break;\
    }\
}

#define CALC_SIGN(result) state->cpsr = (result) & 0x80000000 ? (state->cpsr | CPSR_SIGN) : (state->cpsr & ~CPSR_SIGN);
#define CALC_ZERO(result) state->cpsr = (result) == 0 ? (state->cpsr | CPSR_ZERO) : (state->cpsr & ~CPSR_ZERO);
#define SET_CARRY(carry) state->cpsr = (carry) ? (state->cpsr | CPSR_CARRY) : (state->cpsr & ~CPSR_CARRY);

#define CALC_OVERFLOW_ADD(result, operand1, operand2) {\
    bool overflow = ((operand1) >> 31 == (operand2) >> 31) && ((result) >> 31 != (operand2) >> 31);\
    state->cpsr = overflow ? (state->cpsr | CPSR_OVERFLOW) : (state->cpsr & ~CPSR_OVERFLOW);\
}

#define CALC_OVERFLOW_SUB(result, operand1, operand2) {\
    bool overflow = ((operand1) >> 31 != (operand2) >> 31) && ((result) >> 31 == (operand2) >> 31);\
    state->cpsr = overflow ? (state->cpsr | CPSR_OVERFLOW) : (state->cpsr & ~CPSR_OVERFLOW);\
}

#define LSL(operand, amount, carry) {\
    if (amount != 0) {\
        for (u32 i = 0; i < (amount); i++) {\
            carry = operand & 0x80000000;\
            operand <<= 1;\
        }\
    }\
}

#define LSR(operand, amount, carry, immediate) {\
    if ((immediate) && (amount == 0)) {\
        amount = 32;\
    }\
    for (u32 i = 0; i < amount; i++) {\
        carry = operand & 1;\
        operand >>= 1;\
    }\
}

#define ASR(operand, amount, carry, immediate) {\
    u32 sign_bit = operand & 0x80000000;\
    if ((immediate) && (amount == 0)) {\
        amount = 32;\
    }\
    for (u32 i = 0; i < amount; i++) {\
        carry = operand & 1;\
        operand = (operand >> 1) | sign_bit;\
    }\
}

#define ROR(operand, amount, carry, immediate) {\
    if ((amount) != 0 || !(immediate)) {\
        for (u32 i = 1; i <= amount; i++) {\
            u32 high_bit = (operand & 1) ? 0x80000000 : 0;\
            operand = (operand >> 1) | high_bit;\
            carry = high_bit == 0x80000000;\
        }\
    } else {\
        bool old_carry = carry;\
        carry = operand & 1;\
        operand = (operand >> 1) | (old_carry ? 0x80000000 : 0);\
    }\
}

// todo: optimize this crap x_X
#define CONDITION_BREAK(condition) {\
    bool execute = false;\
    switch (condition) {\
    case 0x0: execute = (state->cpsr & CPSR_ZERO) == CPSR_ZERO; break;\
    case 0x1: execute = (state->cpsr & CPSR_ZERO) != CPSR_ZERO; break;\
    case 0x2: execute = (state->cpsr & CPSR_CARRY) == CPSR_CARRY; break;\
    case 0x3: execute = (state->cpsr & CPSR_CARRY) != CPSR_CARRY; break;\
    case 0x4: execute = (state->cpsr & CPSR_SIGN) == CPSR_SIGN; break;\
    case 0x5: execute = (state->cpsr & CPSR_SIGN) != CPSR_SIGN; break;\
    case 0x6: execute = (state->cpsr & CPSR_OVERFLOW) == CPSR_OVERFLOW; break;\
    case 0x7: execute = (state->cpsr & CPSR_OVERFLOW) != CPSR_OVERFLOW; break;\
    case 0x8: execute = ((state->cpsr & CPSR_CARRY) == CPSR_CARRY) & ((state->cpsr & CPSR_ZERO) != CPSR_ZERO); break;\
    case 0x9: execute = ((state->cpsr & CPSR_CARRY) != CPSR_CARRY) || ((state->cpsr & CPSR_ZERO) == CPSR_ZERO); break;\
    case 0xA: execute = ((state->cpsr & CPSR_SIGN) == CPSR_SIGN) == ((state->cpsr & CPSR_OVERFLOW) == CPSR_OVERFLOW); break;\
    case 0xB: execute = ((state->cpsr & CPSR_SIGN) == CPSR_SIGN) != ((state->cpsr & CPSR_OVERFLOW) == CPSR_OVERFLOW); break;\
    case 0xC: execute = ((state->cpsr & CPSR_ZERO) != CPSR_ZERO) && (((state->cpsr & CPSR_SIGN) == CPSR_SIGN) == ((state->cpsr & CPSR_OVERFLOW) == CPSR_OVERFLOW)); break;\
    case 0xD: execute = ((state->cpsr & CPSR_ZERO) == CPSR_ZERO) || (((state->cpsr & CPSR_SIGN) == CPSR_SIGN) != ((state->cpsr & CPSR_OVERFLOW) == CPSR_OVERFLOW)); break;\
    case 0xE: execute = true; break;\
    case 0xF: execute = false; break;\
    }\
    if (!execute) {\
        return;\
    }\
}

#define SYNC_ONE {\
    cpu->cycles++;\
}

#define SYNC(address, size, write, type) {\
    int cycles = 1 + cpu->memory.cycles(cpu->memory.object, address, size, write, type);\
    cpu->cycles += cycles;\
}

#endif