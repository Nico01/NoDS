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

#ifndef _ARM_CPU_H_
#define _ARM_CPU_H_

#include "arm_global.h"

typedef enum {
    VER_4,
    VER_5
} arm_version;

typedef enum {
    CYCLE_N,
    CYCLE_S,
    CYCLE_I,
    CYCLE_C
} arm_cycle;

typedef enum {
    SIZE_BYTE = 1,
    SIZE_HWORD = 2,
    SIZE_WORD = 4
} arm_size;

typedef int (*cycle_func)(void* object, u32 address, arm_size size, bool write, arm_cycle type);
typedef u32 (*read_func)(void* object, u32 address);
typedef void (*write_func)(void* object, u32 address, u32 value);

typedef enum {
    EXCPT_RESET = 0,
    EXCPT_UNDEFINED = 4,
    EXCPT_SOFTWARE = 8,
    EXCPT_PREFETCH = 12,
    EXCPT_DATA = 16,
    EXCPT_IRQ = 24,
    EXCPT_FIQ = 28
} arm_exception;

typedef enum {
    CPSR_MODE = 0x1F,
    CPSR_THUMB = 0x20,
    CPSR_FIQ_DISABLE = 0x40,
    CPSR_IRQ_DISABLE = 0x80,
    CPSR_STICKY = 0x08000000,
    CPSR_OVERFLOW = 0x10000000,
    CPSR_CARRY = 0x20000000,
    CPSR_ZERO = 0x40000000,
    CPSR_SIGN = 0x80000000
} arm_cpsr_mask;

typedef enum {
    MODE_USR = 0x10,
    MODE_FIQ = 0x11,
    MODE_IRQ = 0x12,
    MODE_SVC = 0x13,
    MODE_ABT = 0x17,
    MODE_UND = 0x1B,
    MODE_SYS = 0x1F
} arm_mode;

typedef struct {
    void* object;
    cycle_func cycles;
    read_func read_byte;
    read_func read_hword;
    read_func read_word;
    write_func write_byte;
    write_func write_hword;
    write_func write_word;
} arm_memory;

typedef struct {
    u32 r15;
    u32 r[14];
    u32 r_fiq[7];
    u32 r_svc[2];
    u32 r_abt[2];
    u32 r_irq[2];
    u32 r_und[2];
    u32* r_ptr[16];
    u32 cpsr;
    u32 spsr_fiq;
    u32 spsr_svc;
    u32 spsr_abt;
    u32 spsr_irq;
    u32 spsr_und;
    u32 spsr_safe;
    u32* spsr_ptr;
} arm_state;

typedef void (*arm_svc_call)(void* cpu, void* object);

typedef struct {
    arm_state* state;
    arm_memory memory;
    arm_version version;
    u32 base_vector;

    struct {
        void* object;
        arm_svc_call method;
    } svc_handler;

    struct {
        int status;
        u32 opcode[3];
        bool flush;
    } pipeline;

    int cycles;
} arm_cpu;

arm_state* arm_make_state();
arm_cpu* arm_make(arm_version version);
void arm_free(arm_cpu* cpu);
void arm_step(arm_cpu* cpu);
void arm_trigger_irq(arm_cpu* cpu);

#endif
