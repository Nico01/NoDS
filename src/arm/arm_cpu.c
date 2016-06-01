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

#include <stdlib.h>
#include "arm_cpu.h"
#include "arm_macro.h"

void arm4_execute(arm_cpu* cpu, u32 instruction);
void arm4_execute_thumb(arm_cpu* cpu, u16 instruction);

arm_state* arm_make_state()
{
    arm_state* state = calloc(1, sizeof(arm_state));
    state->r_ptr[0] = &state->r[0];
    state->r_ptr[1] = &state->r[1];
    state->r_ptr[2] = &state->r[2];
    state->r_ptr[3] = &state->r[3];
    state->r_ptr[4] = &state->r[4];
    state->r_ptr[5] = &state->r[5];
    state->r_ptr[6] = &state->r[6];
    state->r_ptr[7] = &state->r[7];
    state->r_ptr[15] = &state->r15;
    state->cpsr = MODE_SYS;
    ARM_REMAP(state);
    return state;
}

arm_cpu* arm_make(arm_version version)
{
    arm_cpu* cpu = calloc(1, sizeof(arm_cpu));
    cpu->state = arm_make_state();
    cpu->version = version;
    return cpu;
}

void arm_free(arm_cpu* cpu)
{
    free(cpu->state);
    free(cpu);
}

void arm_step(arm_cpu* cpu)
{
    arm_state* state = cpu->state;
    bool thumb = state->cpsr & CPSR_THUMB;
    if (thumb) {
        state->r15 &= ~1;
        switch (cpu->pipeline.status) {
        case 0:
            cpu->pipeline.opcode[0] = MEM_READ_16(state->r15);
            break;
        case 1:
            cpu->pipeline.opcode[1] = MEM_READ_16(state->r15);
            break;
        case 2:
            cpu->pipeline.opcode[2] = MEM_READ_16(state->r15); 
            arm4_execute_thumb(cpu, cpu->pipeline.opcode[0]);
            break;
        case 3:
            cpu->pipeline.opcode[0] = MEM_READ_16(state->r15);
            arm4_execute_thumb(cpu, cpu->pipeline.opcode[1]);
            break;
        case 4:
            cpu->pipeline.opcode[1] = MEM_READ_16(state->r15);
            arm4_execute_thumb(cpu, cpu->pipeline.opcode[2]);
            break;
        }
    } else {
        state->r15 &= ~3;
        switch (cpu->pipeline.status) {
        case 0:
            cpu->pipeline.opcode[0] = MEM_READ_32(state->r15);
            break;
        case 1:
            cpu->pipeline.opcode[1] = MEM_READ_32(state->r15);
            break;
        case 2:
            cpu->pipeline.opcode[2] = MEM_READ_32(state->r15); 
            arm4_execute(cpu, cpu->pipeline.opcode[0]);
            break;
        case 3:
            cpu->pipeline.opcode[0] = MEM_READ_32(state->r15);
            arm4_execute(cpu, cpu->pipeline.opcode[1]);
            break;
        case 4:
            cpu->pipeline.opcode[1] = MEM_READ_32(state->r15);
            arm4_execute(cpu, cpu->pipeline.opcode[2]);
            break;
        }
    }
    if (cpu->pipeline.flush) {
        FLUSH;
        return;
    }
    state->r15 += thumb ? SIZE_HWORD : SIZE_WORD;
    if (++cpu->pipeline.status == 5) {
        cpu->pipeline.status = 2;
    }
}

void arm_trigger_irq(arm_cpu* cpu)
{
    arm_state* state = cpu->state;
    if (!(state->cpsr & CPSR_IRQ_DISABLE)) {
        state->r_irq[1] = state->r15 - ((state->cpsr & CPSR_THUMB) ? 4 : 8) + SIZE_WORD;
        state->r15 = cpu->base_vector + EXCPT_IRQ;
        state->spsr_irq = state->cpsr;
        state->cpsr = (state->cpsr & ~(CPSR_MODE | CPSR_THUMB)) | MODE_IRQ | CPSR_IRQ_DISABLE;
        ARM_REMAP(state);
        FLUSH;
    }
}

