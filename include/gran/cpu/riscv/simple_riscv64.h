/* SPDX-License-Identifier: copyleft-next-0.3.1 */
/* Copyright 2023 Kim Kuparinen < kimi.h.kuparinen@gmail.com > */

#ifndef GRAN_SIMPLE_RISCV64
#define GRAN_SIMPLE_RISCV64

#include <gran/component.h>

struct component *create_simple_riscv64(uint64_t rcv, uint64_t start_pc,
                                        struct component *imem,
                                        struct component *dmem);

void simple_riscv64_set_reg(struct component *cpu, size_t reg, uint64_t val);

#endif /* GRAN_SIMPLE_RISCV */
