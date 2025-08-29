#include <stdlib.h>
#include <gran/mem/ideal_alloc.h>

struct region {
	uint64_t start, end;
};

static inline int region_cmp(struct region a, struct region b)
{
	if (a.start == b.start && a.end == b.end)
		return 0;

	if (a.start > b.start)
		return 1;

	if (a.end < b.end)
		return -1;

	return 0;
}

#define SPTREE_NAME regions
#define SPTREE_TYPE struct region
#define SPTREE_CMP(a, b) region_cmp((a), (b))
#include <conts/sptree.h>

struct space {
	struct regions regions;
};

static struct space create_space()
{
	return (struct space){.regions = regions_create()};
}

static void destroy_space(struct space *space)
{
	regions_destroy(&space->regions);
}

#define VEC_NAME spaces
#define VEC_TYPE struct space
#include <conts/vec.h>

struct alloc {
	struct component component;
	struct spaces spaces;
	uint64_t max_spaces;

	struct reg in;
	struct reg out;

	struct component *send;
};

static void alloc_destroy(struct alloc *a)
{
	foreach(spaces, s, &a->spaces) {
		destroy_space(s);
	}

	spaces_destroy(&a->spaces);
	free(a);
}

static stat alloc_receive(struct alloc *a, struct component *from,
                          struct packet pkt)
{
	(void)from; /* unused */
	if (a->in.busy)
		return EBUSY;

	a->in.pkt = pkt;
	a->in.busy = true;
	return OK;
}

static stat alloc_resp(struct alloc *a, enum alloc_if_ret resp, uint64_t start,
                       uint64_t end)
{
	a->in.busy = false;

	struct packet pkt = response(a->in.pkt);
	struct alloc_if_r *r = (struct alloc_if_r *)&pkt.data;
	r->r = resp;
	r->start = start;
	r->end = end;

	stat ret = SEND(a, a->send, pkt);
	if (ret == EBUSY) {
		a->out.pkt = pkt;
		a->out.busy = true;
		return OK;
	}

	return ret;
}

static stat alloc_new(struct alloc *a)
{
	struct alloc_if_new *n = (struct alloc_if_new *)&a->in.pkt.data;
	if (n->space >= a->max_spaces)
		return alloc_resp(a, ALLOC_IF_ERR_SPACE, 0, 0);

	struct space *space = spaces_at(&a->spaces, n->space);
	assert(space);

	struct region new = {.start = n->start, .end = n->end};
	struct region *region = regions_insert(&space->regions, new);
	if (!region) {
		alloc_resp(a, ALLOC_IF_ERR_RANGE, 0, 0);
		return EMEM;
	}

	return alloc_resp(a, ALLOC_IF_OK, n->start, n->end);
}

static stat alloc_rm(struct alloc *a)
{
	struct alloc_if_rm *rm = (struct alloc_if_rm *)&a->in.pkt.data;
	if (rm->space >= a->max_spaces)
		return alloc_resp(a, ALLOC_IF_ERR_SPACE, 0, 0);

	struct space *space = spaces_at(&a->spaces, rm->space);
	assert(space);

	struct region find = {.start = rm->start, .end = rm->end};
	struct region *region = regions_find(&space->regions, find);
	if (!region)
		return alloc_resp(a, ALLOC_IF_ERR_RANGE, 0, 0);

	regions_remove_found(&space->regions, region);
	regions_free_found(&space->regions, region);
	return alloc_resp(a, ALLOC_IF_OK, 0, 0);
}

static stat alloc_q(struct alloc *a)
{
	struct alloc_if_q *q = (struct alloc_if_q *)&a->in.pkt.data;
	if (q->space >= a->max_spaces)
		return alloc_resp(a, ALLOC_IF_ERR_SPACE, 0, 0);

	struct space *space = spaces_at(&a->spaces, q->space);
	assert(space);

	struct region find = {.start = q->addr, .end = q->addr};
	struct region *region = regions_find(&space->regions, find);
	if (!region)
		return alloc_resp(a, ALLOC_IF_ERR_RANGE, 0, 0);

	return alloc_resp(a, ALLOC_IF_OK, region->start, region->end);
}

static stat alloc_clock(struct alloc *a)
{
	if (!a->in.busy)
		return OK;

	if (a->out.busy) {
		stat ret = SEND(a, a->send, a->out.pkt);
		if (ret == EBUSY)
			return OK;

		assert(ret == OK);
		a->out.busy = false;
	}

	uint32_t off = (uint32_t)a->in.pkt.to;
	switch (off) {
	case ALLOC_IF_NEW: return alloc_new(a);
	case ALLOC_IF_RM: return alloc_rm(a);
	case ALLOC_IF_Q: return alloc_q(a);
	default: break;
	}

	return OK;
}

void ideal_alloc_connect(struct component *alloc, struct component *send)
{
	struct alloc *a = (struct alloc *)alloc;
	assert(a->send == NULL);
	a->send = send;
}

struct component *create_ideal_alloc(uint64_t max_spaces)
{
	struct alloc *a = calloc(1, sizeof(struct alloc));
	a->spaces = spaces_create(max_spaces);
	for (size_t i = 0; i < max_spaces; ++i) {
		struct space space = create_space();
		if (!spaces_append(&a->spaces, space)) {
			spaces_destroy(&a->spaces);
			return NULL;
		}
	}

	a->max_spaces = max_spaces;
	a->component.clock = (clock_callback)alloc_clock;
	a->component.receive = (receive_callback)alloc_receive;
	a->component.destroy = (destroy_callback)alloc_destroy;
	return (struct component *)a;
}
