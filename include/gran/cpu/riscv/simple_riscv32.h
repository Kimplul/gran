/* SPDX-License-Identifier: copyleft-next-0.3.1 */
/* Copyright 2023 Kim Kuparinen < kimi.h.kuparinen@gmail.com > */

#ifndef GRAN_SIMPLE_RISCV32
#define GRAN_SIMPLE_RISCV32

#include <gran/component.h>

struct component *create_simple_riscv32(uint32_t start_pc,
                                        struct component *imem,
                                        struct component *dmem);

#endif /* GRAN_SIMPLE_RISCV */
