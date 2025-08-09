#include <gran/if/alloc.h>
#include "testbench.h"

struct tb {
	struct component component;
	struct component *dut;
	struct reg in;

	enum {
		ADD_REGION,
		QUERY_EXISTING,
		REMOVE_REGION,
		QUERY_NONEXISTING,
		FINAL
	} state;
};

static stat tb_receive(struct tb *tb, struct component *from, struct packet pkt)
{
	assert(tb->dut == from);
	tb->in.pkt = pkt;
	tb->in.busy = true;
	return OK;
}

static stat tb_clock(struct tb *tb)
{
	assert(tb->dut);

	switch (tb->state) {
	case ADD_REGION: {
		struct alloc_if_new n = {.space = 0, .start = 0, .end = 10};
		struct packet pkt = create_packet(0, ALLOC_IF_NEW, sizeof(n),
		                                  &n, PACKET_WRITE);
		assert(SEND(tb, tb->dut, pkt) == OK);
		tb->state = QUERY_EXISTING;
		break;
	}

	case QUERY_EXISTING: {
		if (!tb->in.busy)
			break;

		struct alloc_if_r *r = (struct alloc_if_r *)&tb->in.pkt.data;
		assert(r->r == ALLOC_IF_OK);

		struct alloc_if_q q = {.space = 0, .addr = 5};
		struct packet pkt = create_packet(0, ALLOC_IF_Q, sizeof(q), &q,
		                                  PACKET_WRITE);
		assert(SEND(tb, tb->dut, pkt) == OK);
		tb->state = REMOVE_REGION;
		tb->in.busy = false;
		break;
	}

	case REMOVE_REGION: {
		if (!tb->in.busy)
			break;

		struct alloc_if_r *r = (struct alloc_if_r *)&tb->in.pkt.data;
		assert(r->r == ALLOC_IF_OK);
		assert(r->start == 0);
		assert(r->end == 10);

		struct alloc_if_rm rm = {.space = 0, .start = 0, .end = 10};
		struct packet pkt = create_packet(0, ALLOC_IF_RM, sizeof(rm),
		                                  &rm, PACKET_WRITE);
		assert(SEND(tb, tb->dut, pkt) == OK);
		tb->state = QUERY_NONEXISTING;
		tb->in.busy = false;
		break;
	}

	case QUERY_NONEXISTING: {
		if (!tb->in.busy)
			break;

		struct alloc_if_r *r = (struct alloc_if_r *)&tb->in.pkt.data;
		assert(r->r == ALLOC_IF_OK);

		struct alloc_if_q q = {.space = 0, .addr = 5};
		struct packet pkt = create_packet(0, ALLOC_IF_Q, sizeof(q), &q,
		                                  PACKET_WRITE);
		assert(SEND(tb, tb->dut, pkt) == OK);
		tb->state = FINAL;
		tb->in.busy = false;
		break;
	}

	case FINAL: {
		if (!tb->in.busy)
			break;

		struct alloc_if_r *r = (struct alloc_if_r *)&tb->in.pkt.data;
		assert(r->r != ALLOC_IF_OK);
		return DONE;
	}

	default: abort();
	}

	return OK;
}

struct component *create_testbench()
{
	struct tb *tb = calloc(1, sizeof(struct tb));
	tb->component.clock = (clock_callback)tb_clock;
	tb->component.receive = (receive_callback)tb_receive;
	return (struct component *)tb;
}

stat testbench_connect(struct component *tb, struct component *dut)
{
	struct tb *t = (struct tb *)tb;
	t->dut = dut;
	return OK;
}
