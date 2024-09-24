/* SPDX-License-Identifier: copyleft-next-0.3.1 */
/* Copyright 2023 Kim Kuparinen < kimi.h.kuparinen@gmail.com > */

#include <stdlib.h>
#include <assert.h>

#include <gran/component.h>

#include "traffic_gen.h"

struct traffic_gen {
	struct component component;
	struct component *stress;

	struct packet pkt;

	uint64_t rcv;
	uint64_t addr;
	uint64_t end;

	size_t counter;
};

static stat traffic_gen_receive(struct traffic_gen *tg, struct component *from, struct packet pkt)
{
	(void)from;
	tg->pkt = pkt;
	return OK;
}

static stat traffic_gen_clock(struct traffic_gen *tg)
{
	if (tg->addr == tg->end)
		return DONE;

	uint8_t c = 0;
	switch (tg->counter) {
	case 0:
		c = 13;
		tg->pkt = create_packet(tg->rcv, tg->addr, sizeof(c), &c, PACKET_WRITE);
		assert(SEND(tg, tg->stress, tg->pkt) == OK);
		tg->counter++;
		break;

	case 1:
		/* wait for write to finish */
		if (!is_set(&tg->pkt, PACKET_DONE))
			break;

		tg->counter++;
		break;

	case 2:
		tg->pkt = create_packet(0, tg->addr, sizeof(c), NULL, PACKET_READ);
		assert(SEND(tg, tg->stress, tg->pkt) == OK);
		tg->counter++;
		break;

	case 3:
		/* wait for read to finish */
		if (!is_set(&tg->pkt, PACKET_DONE))
			break;

		tg->counter++;
		break;

	case 4:
		c = packet_convu8(&tg->pkt);
		assert(c == 13);
		tg->counter = 0;
		tg->addr++;
		break;
	}

	return OK;
}

struct component *create_traffic_gen(struct component *stress, uintptr_t start, size_t size)
{
	struct traffic_gen *new = calloc(1, sizeof(struct traffic_gen));
	if (!new)
		return NULL;

	new->stress = stress;
	new->addr = start;
	new->end = start + size;
	new->counter = 0;

	new->component.receive = (receive_callback)traffic_gen_receive;
	new->component.clock = (clock_callback)traffic_gen_clock;
	return (struct component *)new;
}
