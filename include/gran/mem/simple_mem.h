/* SPDX-License-Identifier: copyleft-next-0.3.1 */
/* Copyright 2023 Kim Kuparinen < kimi.h.kuparinen@gmail.com > */

#ifndef GRAN_SIMPLE_MEM_H
#define GRAN_SIMPLE_MEM_H

#include <gran/component.h>

struct component *create_simple_mem(size_t size);
void init_simple_mem(struct component *mem, uintptr_t addr, size_t size,
                     void *data);

#endif /* GRAN_SIMPLE_MEM_H */
