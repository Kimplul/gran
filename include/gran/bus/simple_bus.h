/* SPDX-License-Identifier: copyleft-next-0.3.1 */
/* Copyright 2023 Kim Kuparinen < kimi.h.kuparinen@gmail.com > */

#ifndef GRAN_SIMPLE_BUS_H
#define GRAN_SIMPLE_BUS_H

#include <stdint.h>

struct component *create_simple_bus();
stat simple_bus_add(struct component *bus, struct component *component,
                    uint64_t addr, uint64_t size);

#endif /* GRAN_SIMPLE_BUS_H */
