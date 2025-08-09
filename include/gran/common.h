/* SPDX-License-Identifier: copyleft-next-0.3.1 */
/* Copyright 2023 Kim Kuparinen < kimi.h.kuparinen@gmail.com > */

#ifndef GRAN_COMMON_H
#define GRAN_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include "packet.h"

typedef enum {
	OK = 0,
	DONE,
	EBUS,
	EBUSY,
	EMEM,
	ENOMEM,
	EEXISTS,
	ENOSUCH,
	ESIZE,
} stat;

#define error(x, ...) \
	do {fprintf(stderr, "error: " x "\n",##__VA_ARGS__);} while(0)

#define warn(x, ...) \
	do {fprintf(stderr, "warn: " x "\n",##__VA_ARGS__);} while(0)

#define info(x, ...) \
	do {fprintf(stderr, "info: " x "\n",##__VA_ARGS__);} while(0)

#if DEBUG
#define debug(x, ...) \
	do {fprintf(stderr, "debug: " x "\n",##__VA_ARGS__);} while(0)
#else
#define debug(x, ...)
#endif

struct reg {
	struct packet pkt;
	bool busy;
};

/* Places \p pkt into \p r if \r is not busy. Returns EBUSY if register was
 * busy, otherwise OK. */
stat place_reg(struct reg *r, struct packet pkt);

/* If \p is free, copies packet from \p s to \p r, marking each correspondigly
 * free/busy. Returns EBUSY if \r was busy, otherwise OK. */
stat copy_reg(struct reg *r, struct reg *s);

/* For each register in \p in, calls \p sel to check if packet in registers
 * should be moved to \p out. If there are multiple available registers, oldest
 * one is selected. NULL entries in \p in are tolerated. */
void propagate(struct reg *out,
               size_t count, struct reg *in[static count],
               bool (*sel)(struct reg *r, void *data), void *data);

#endif /* GRAN_COMMON_H */
