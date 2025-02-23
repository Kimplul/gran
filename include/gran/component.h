/* SPDX-License-Identifier: copyleft-next-0.3.1 */
/* Copyright 2023 Kim Kuparinen < kimi.h.kuparinen@gmail.com > */

#ifndef GRAN_COMPONENT_H
#define GRAN_COMPONENT_H

#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <inttypes.h>

#include <gran/common.h>
#include <gran/packet.h>

struct component;

typedef stat (*receive_callback)(struct component *to, struct component *from, struct packet pkt);
typedef stat (*clock_callback)(struct component *);
typedef stat (*stat_callback)(struct component *, FILE *);
typedef stat (*dts_callback)(struct component *, FILE *);
typedef void (*destroy_callback)(struct component *);

struct component {
	/* optional */
	char *name;

	receive_callback receive;
	clock_callback clock;

	dts_callback dts;
	stat_callback stat;

	destroy_callback destroy;
};

static inline stat send(struct component *from, struct component *to, struct packet pkt)
{
	assert(to && from);
	if (!to->receive) {
		/* printf formatted asserts would maybe be preferable? */
		error(
			"%s:%lx tried sending to %s:%lx",
			from->name, pkt.from,
			to->name, pkt.to);
		return ENOSUCH;
	}

	return to->receive(to, from, pkt);
}
#define SEND(x, y, z) send((struct component *)(x), (struct component *)(y), z)

static inline void destroy(struct component *component)
{
	free(component->name);

	if (component->destroy)
		component->destroy(component);
	else
		free(component);
}

#endif /* GRAN_COMPONENT_H */
