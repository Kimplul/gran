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
#include <gran/snoop.h>

struct component;

typedef stat (*read_callback)(struct component *, struct packet *pkt);
typedef stat (*write_callback)(struct component *, struct packet *pkt);
typedef stat (*swap_callback)(struct component *, struct packet *pkt);

typedef stat (*snoop_callback)(struct component *, struct snoop *snoop);
typedef stat (*ctrl_callback)(struct component *, struct packet *pkt);

typedef stat (*irq_callback)(struct component *, int);
typedef stat (*clock_callback)(struct component *);
typedef stat (*stat_callback)(struct component *, FILE *);
typedef stat (*dts_callback)(struct component *, FILE *);

typedef void (*destroy_callback)(struct component *);

struct component {
	/* optional */
	char *name;

	read_callback read;
	write_callback write;
	swap_callback swap;

	snoop_callback snoop;
	ctrl_callback ctrl;

	irq_callback irq;
	clock_callback clock;

	dts_callback dts;
	stat_callback stat;

	destroy_callback destroy;
};

static inline stat write(struct component *component, struct packet *pkt)
{
	assert(packet_type(pkt) == PACKET_WRITE);
	if (!component->write) {
		/* printf formatted asserts would maybe be preferable? */
		error(
			"tried writing %p:%" PRIuPTR " but it doesn't support writing",
			component->name, packet_addr(pkt));
		return ENOSUCH;
	}

	packet_set_state(pkt, PACKET_SENT);
	return component->write(component, pkt);
}

static inline stat read(struct component *component, struct packet *pkt)
{
	assert(packet_type(pkt) == PACKET_READ);
	if (!component->read) {
		error(
			"tried reading %p:%" PRIuPTR " but it doesn't support reading",
			component->name, packet_addr(pkt));
		return ENOSUCH;
	}

	packet_set_state(pkt, PACKET_SENT);
	return component->read(component, pkt);
}

static inline stat swap(struct component *component, struct packet *pkt)
{
	assert(packet_type(pkt) == PACKET_SWAP);
	if (!component->swap) {
		error(
			"tried swapping %p:%" PRIuPTR " but it doesn't support swapping",
			component->name, packet_addr(pkt));
		return ENOSUCH;
	}

	packet_set_state(pkt, PACKET_SENT);
	return component->swap(component, pkt);
}

static inline void destroy(struct component *component)
{
	free(component->name);

	if (component->destroy)
		component->destroy(component);
	else
		free(component);
}

#endif /* GRAN_COMPONENT_H */
