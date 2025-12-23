#include <gran/cache/simt_cache.h>

#define VEC_NAME lanes
#define VEC_TYPE struct component *
#include <conts/vec.h>

struct req {
	struct reg reg;
	bool queued;
};

#define VEC_NAME reqs
#define VEC_TYPE struct req
#include <conts/vec.h>

struct simt_cache {
	struct component component;
	struct component *mem;
	struct lanes lanes;
	struct reqs reqs;
	uint64_t rcv;
	uint32_t rr;
};

static stat simt_cache_ext_send(struct simt_cache *cache, size_t i, struct req *r)
{
	assert(r->reg.busy);

	/* do fixups for sending */
	struct packet p = r->reg.pkt;
	if (is_set(&p, PACKET_READ))
		p.mask = ~0ULL;

	p.from = cache->rcv | i;
	return SEND(&cache->component, cache->mem, p);
}

static stat simt_cache_clock(struct simt_cache *cache)
{
	struct req *shared = reqs_at(&cache->reqs, cache->rr);

	/* start sending out queued stuff */
	for (size_t i = 0; i < reqs_len(&cache->reqs); ++i) {
		struct req *r = reqs_at(&cache->reqs, i);
		if (!r->queued)
			continue;

		if (!r->reg.busy)
			continue;

		struct packet *p = &r->reg.pkt;

		/* if we're trying to read from the same address as the shared reg and
		 * we're not currently in charge of the shared reg, we can stop queuing
		 * as shared reg will take care of our request as well */
		if (shared->reg.busy
				&& is_set(&shared->reg.pkt, PACKET_READ)
				&& is_set(p, PACKET_READ)
				&& p->to == shared->reg.pkt.to
				&& i != cache->rr) {
			r->queued = false;
			continue;
		}

		size_t block_idx = (p->to / 64) % reqs_len(&cache->reqs);

		/* check if the address in this packet can be directly sent to
		 * the corresponding block port, i.e. the 64 byte block matches
		 * our index. Alternatively, if we're currently holding the
		 * shared register, we can send to any block index */
		if (block_idx == i || cache->rr == i) {
			stat ok = simt_cache_ext_send(cache, i, r);
			if (ok == OK) {
				r->queued = false;
				continue;
			}

			if (ok == EBUSY)
				continue;

			/* something went wrong */
			return ok;
		}
	}

	/* if we're not waiting on a shared request, move forward to next active
	 * element to prevent deadlocks */
	if (!shared->reg.busy)
	for (size_t i = 0; i < reqs_len(&cache->reqs); ++i) {
		cache->rr = (cache->rr + 1) % reqs_len(&cache->reqs);
		if (reqs_at(&cache->reqs, cache->rr)->reg.busy)
			break;
	}

	return OK;
}

static stat simt_cache_broadcast(struct simt_cache *cache, struct packet pkt)
{
	assert(is_set(&pkt, PACKET_READ));
	for (size_t i = 0; i < reqs_len(&cache->reqs); ++i) {
		struct req *r = reqs_at(&cache->reqs, i);
		if (!r->reg.busy)
			continue;

		if (r->reg.pkt.to != pkt.from)
			continue;

		if (!is_set(&r->reg.pkt, PACKET_READ))
			continue;

		struct component *lane = *lanes_at(&cache->lanes, i);

		pkt.mask = r->reg.pkt.mask;
		pkt.to   = r->reg.pkt.from;

		stat ok = SEND(&cache->component, lane, pkt);

		/* no reason for core to be blocked */
		assert(ok == OK);

		r->reg.busy = false;

		/* queued might be set at this point if there was an attempt to
		 * send a packet that failed, but we can deal with it here */
		r->queued   = false;
	}

	return OK;
}

static stat simt_cache_handle_response(struct simt_cache *cache, struct packet pkt)
{
	/** @todo handle external requests, this currently only accepts
	 * responses and an IPI would not work with this */
	assert(!is_set(&pkt, PACKET_ERROR));
	assert(is_set(&pkt, PACKET_DONE));
	assert(is_set(&pkt, PACKET_READ) ? pkt.mask == ~0ULL : 1);

	uint32_t idx = (uint32_t)pkt.to;
	if (cache->rr == idx) {
		/* round robin satisfied, move to next element */
		cache->rr = (cache->rr + 1) % reqs_len(&cache->reqs);
		if (is_set(&pkt, PACKET_READ))
			return simt_cache_broadcast(cache, pkt);
	}

	struct req *r = reqs_at(&cache->reqs, idx);
	assert(!r->queued);
	assert(r->reg.busy);
	assert(r->reg.pkt.to == pkt.from);

	/* restore rewritten fields */
	pkt.mask = r->reg.pkt.mask;
	pkt.to   = r->reg.pkt.from;

	r->reg.busy = false;

	struct component *lane = *lanes_at(&cache->lanes, idx);
	stat ok = SEND(&cache->component, lane, pkt);
	assert(ok == OK);
	return ok;
}

static stat simt_cache_receive(struct simt_cache *cache,
		struct component *from, struct packet pkt)
{
	if (cache->mem == from)
		return simt_cache_handle_response(cache, pkt);

	uint32_t idx = pkt.from >> 32;
	struct component *lane = *lanes_at(&cache->lanes, idx);
	if (lane != from)
		return ENOSUCH;

	struct req *r = reqs_at(&cache->reqs, idx);
	if (r->reg.busy)
		return EBUSY;

	assert(r->queued == false);
	r->reg.busy = true;
	r->reg.pkt = pkt;
	r->queued = true;

	return OK;
}

static void simt_cache_destroy(struct simt_cache *cache)
{
	lanes_destroy(&cache->lanes);
	reqs_destroy(&cache->reqs);
	free(cache);
}

struct component *create_simt_cache(uint64_t rcv, size_t num_lanes, struct component *mem)
{
	struct simt_cache *cache = calloc(1, sizeof(struct simt_cache));
	if (!cache)
		return NULL;

	cache->component.clock = (clock_callback)simt_cache_clock;
	cache->component.receive = (receive_callback)simt_cache_receive;
	cache->component.destroy = (destroy_callback)simt_cache_destroy;

	cache->lanes = lanes_create(num_lanes);
	cache->reqs  = reqs_create(num_lanes);
	cache->mem   = mem;
	cache->rcv   = rcv;
	cache->rr    = 0;

	struct req empty = {
		.reg = {
			.pkt = {},
			.busy = false
		},
		.queued = false
	};

	for (size_t i = 0; i < num_lanes; ++i) {
		reqs_append(&cache->reqs, empty);
		lanes_append(&cache->lanes, NULL);
	}

	return &cache->component;
}

void simt_cache_connect_lane(struct component *c, size_t i, struct component *e)
{
	struct simt_cache *cache = (struct simt_cache *)c;
	*lanes_at(&cache->lanes, i) = e;
}
