/* SPDX-License-Identifier: copyleft-next-0.3.1 */
/* Copyright 2023 Kim Kuparinen < kimi.h.kuparinen@gmail.com > */

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include <gran/mem/simple_mem.h>

struct simple_mem {
	struct component component;
	struct component *send;
	struct packet pkt;
	bool busy;

	size_t size;
	uint8_t buf[];
};

static stat simple_mem_clock(struct simple_mem *mem)
{
	if (!mem->busy)
		return OK;

	stat r = SEND(mem, mem->send, mem->pkt);
	if (r == EBUSY)
		return OK;

	mem->busy = false;
	return OK;
}

static stat simple_mem_receive(struct simple_mem *mem, struct component *from,
                               struct packet pkt)
{
	if (mem->busy)
		return EBUSY;

	mem->busy = true;
	mem->send = from;

	uint64_t offset = pkt.to % mem->size;
	if (offset >= mem->size) {
		error("read outside memory");
		mem->pkt = response(pkt);
		set_flags(&mem->pkt, PACKET_ERROR);
		return OK;
	}

	if (is_set(&pkt, PACKET_READ))
		checked_copyto(&pkt, mem->buf + offset);
	else if (is_set(&pkt, PACKET_WRITE))
		checked_copyfrom(&pkt, mem->buf + offset);
	else
		abort();

	mem->pkt = response(pkt);
	set_flags(&mem->pkt, PACKET_DONE);
	return OK;
}

struct component *create_simple_mem(size_t size)
{
	struct simple_mem *new = calloc(1, sizeof(struct simple_mem) + size);
	if (!new)
		return NULL;

	new->size = size;
	new->component.receive = (receive_callback)simple_mem_receive;
	new->component.clock = (clock_callback)simple_mem_clock;
	return (struct component *)new;
}

void init_simple_mem(struct component *m, uintptr_t addr, size_t size,
                     void *data)
{
	struct simple_mem *mem = (struct simple_mem *)m;
	memcpy(mem->buf + addr, data, size);
}
