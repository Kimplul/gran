/* SPDX-License-Identifier: copyleft-next-0.3.1 */
/* Copyright 2023 Kim Kuparinen < kimi.h.kuparinen@gmail.com > */

#include <stdlib.h>
#include <assert.h>

#include <gran/component.h>

#include "traffic_gen.h"

struct traffic_gen {
	struct component component;
	struct component *stress;

	struct packet *pkt;

	uintptr_t addr;
	uintptr_t end;

	size_t counter;
};

static stat traffic_gen_clock(struct traffic_gen *tg)
{
	if (tg->addr == tg->end)
		return DONE;

	uint8_t c = 0;
	if (tg->pkt) {
		assert(packet_state(tg->pkt) != PACKET_FAILED);
		/* wait for packet */
		if (packet_state(tg->pkt) != PACKET_DONE)
			return OK;
	}

	switch (tg->counter) {
	case 0:
		c = 13;
		tg->pkt = create_packet(PACKET_WRITE, tg->addr, sizeof(c));
		*(uint8_t *)packet_data(tg->pkt) = c;
		assert(write(tg->stress, tg->pkt) == OK);
		tg->counter++;
		break;

	case 1:
		/* destroy write packet now that the write is done */
		destroy_packet(tg->pkt);
		tg->pkt = NULL;
		tg->counter++;
		break;

	case 2:
		tg->pkt = create_packet(PACKET_READ, tg->addr, sizeof(c));
		assert(read(tg->stress, tg->pkt) == OK);
		tg->counter++;
		break;

	case 3:
		c = *(uint8_t *)packet_data(tg->pkt);
		destroy_packet(tg->pkt);
		tg->pkt = NULL;
		tg->counter = 0;
		tg->addr++;
		break;
	}

	return OK;
}

void traffic_gen_destroy(struct traffic_gen *tg)
{
	if (tg->pkt)
		destroy_packet(tg->pkt);

	destroy(tg->stress);
	free(tg);
}

struct component *create_traffic_gen(struct component *stress, uintptr_t start,
                                     size_t size)
{
	struct traffic_gen *new = calloc(1, sizeof(struct traffic_gen));
	if (!new)
		return NULL;

	new->stress = stress;
	new->addr = start;
	new->end = start + size;
	new->counter = 0;

	new->component.clock = (clock_callback)traffic_gen_clock;
	new->component.destroy = (destroy_callback)traffic_gen_destroy;
	return (struct component *)new;
}
