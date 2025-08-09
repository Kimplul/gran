#include <gran/ideal_noc.h>

struct noc {
	struct component component;
	uint32_t elems;
	size_t latency;

	size_t counter;

	struct reg *in; /* countedby[elems] */
	struct reg *out; /* countedby[elems] */
	struct component **lower; /* countedby[elems] */
};

static stat ideal_noc_receive(struct noc *n, struct component *from,
                              struct packet pkt)
{
	(void)from; /* unused */
	uint32_t elem;
	addr_ideal_noc(pkt.to, &elem, NULL);
	assert(elem < n->elems);

	if (n->in[elem].busy)
		return EBUSY;

	pkt.timestamp = n->counter;
	n->in[elem].pkt = pkt;
	n->in[elem].busy = true;
	return OK;
}

static stat ideal_noc_clock(struct noc *n)
{
	size_t counter = n->latency == 0 ? 0 : (n->counter + 1) % n->latency;
	if (counter != 0)
		return OK;

	for (size_t i = 0; i < n->elems; ++i) {
		struct reg *in = &n->in[i];
		if (!in->busy)
			continue;

		uint32_t elem;
		addr_ideal_noc(in->pkt.to, &elem, NULL);
		assert(elem < n->elems);

		struct reg *out = &n->out[elem];
		if (out->busy && out->pkt.timestamp < in->pkt.timestamp)
			continue;

		out->pkt = in->pkt;
		out->busy = true;
		in->busy = false;
	}

	for (size_t i = 0; i < n->elems; ++i) {
		struct reg *out = &n->out[i];
		if (!out->busy)
			continue;

		uint32_t elem;
		addr_ideal_noc(out->pkt.to, &elem, NULL);
		struct component *lower = n->lower[elem];
		assert(lower);

		stat ret = SEND(n, lower, out->pkt);
		if (ret == EBUSY)
			continue;

		assert(ret == OK);
		out->busy = false;
	}

	return OK;
}

static void ideal_noc_destroy(struct noc *n)
{
	free(n->in);
	free(n->out);
	free(n->lower);
	free(n);
}

stat ideal_noc_connect(struct component *node, struct component *component,
                       uint32_t elem)
{
	struct noc *n = (struct noc *)node;
	assert(elem < n->elems);
	assert(n->lower[elem] == NULL);
	n->lower[elem] = component;
	return OK;
}

struct component *create_ideal_noc(uint32_t elems, size_t latency)
{
	struct noc *n = (struct noc *)calloc(1, sizeof(struct noc));
	if (!n)
		return NULL;

	n->in = (struct reg *)calloc(elems, sizeof(struct reg));
	if (!n->in) {
		free(n);
		return NULL;
	}

	n->out = (struct reg *)calloc(elems, sizeof(struct reg));
	if (!n->out) {
		free(n->in);
		free(n);
		return NULL;
	}

	n->lower = (struct component **)calloc(elems,
	                                       sizeof(struct component *));
	if (!n->lower) {
		free(n->out);
		free(n->in);
		free(n);
		return NULL;
	}

	n->component.destroy = (destroy_callback)ideal_noc_destroy;
	n->component.receive = (receive_callback)ideal_noc_receive;
	n->component.clock = (clock_callback)ideal_noc_clock;
	n->latency = latency;
	n->elems = elems;
	return (struct component *)n;
}
