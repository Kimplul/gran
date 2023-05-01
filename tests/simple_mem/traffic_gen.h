/* SPDX-License-Identifier: copyleft-next-0.3.1 */
/* Copyright 2023 Kim Kuparinen < kimi.h.kuparinen@gmail.com > */

#ifndef GRAN_TRAFFIC_GEN_H
#define GRAN_TRAFFIC_GEN_H

#include <stdint.h>
#include <stddef.h>

struct component *create_traffic_gen(struct component *, uintptr_t, size_t);

#endif /* GRAN_TRAFFIC_GEN_H */
