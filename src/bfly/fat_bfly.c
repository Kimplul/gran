#include <gran/utils.h>
#include <gran/bfly/fat_bfly.h>

/*
 * Essentially, we build up a binary tree of nodes, where each node has two
 * input 'banks' of width 2^height, and an output 'bank' twice that. The highest
 * node has an 'output bank' the same width as the number of nodes in the
 * network, and data flows upwards in this tree such that packets automatically
 * end up in the correct order in the final output bank. From there, we can just
 * directly write the output registers to child nodes. Something like the
 * following:
 *
 *   ||||
 *   0123    - height 2, 2^2 nodes with 2*2^1 input registers
 *  //   \\
 *  01   23  - height 1, 2^1 nodes with 2*2^0 input registers
 * /  \ /  \
 * 0  1 2  3 - height 0, input layer
 *
 * The cool thing with this approach is that each output register has to look at just
 * two input registers, since the input is always (partially) sorted.
 *
 * The implementation is a bit nontrivial, unfortunately. Another possibility
 * could be to initialize a plain 2D array with registers and do some index
 * calculations to directly associate registers with eachother (our nodes are
 * ultimately just wrappers around some registers, along with some selection
 * logic) that I tried to go for, but there are some subtleties in how data
 * flows in the tree that caused my other attempt to deadlock frequently.
 * Shouldn't be impossible to fix, but there was even more weird bit math than
 * in this version so I'm sticking with this for now at least.
 */

struct reg {
	struct packet pkt;
	bool busy;
};

struct fat_node {
	/* parent in tree */
	struct fat_node *up;

	/* arrays, length is calculated based on the node height */
	struct reg *left, *right;
	struct reg *out;
};

struct fat_bfly {
	struct component component;

	uint32_t elems;
	struct reg *in; /* elems */
	struct component **send; /* elems */

	size_t layers;
	/* noe that in this case output is layer 0 and input layers - 1 */
	struct fat_node **layer;
};

/* left selects which input bank to use and idx which index within it */
static stat node_receive(struct fat_node *node, struct packet pkt, size_t idx, bool left)
{
	struct reg *rs = left ? node->left : node->right;
	if (rs[idx].busy)
		return EBUSY;

	rs[idx].pkt = pkt;
	rs[idx].busy = true;
	return OK;
}

static stat node_clock(struct fat_node *node, size_t count, size_t height)
{
	/* can't go higher than the top layer */
	if (node->up)
	for (size_t i = 0; i < count; ++i) {
		if (!node->out[i].busy)
			continue;

		struct packet *pkt = &node->out[i].pkt;

		uint32_t elem;
		addr_fat_bfly(pkt->to, &elem, NULL);

		/* here's a pretty tricky spot, note how we select the input
		 * bank based on the address of our target. This increases
		 * hardware complexity, but avoids at least one possible
		 * deadlock situation compared to just having a fixed right/left
		 * selector. */
		stat ret = node_receive(node->up, *pkt, i, (elem >> height) & 1);
		if (ret == EBUSY)
			continue;

		node->out[i].busy = false;
	}

	/* populate output registers */
	for (size_t i = 0; i < count; ++i) {
		/* waiting on something upstream */
		if (node->out[i].busy)
			continue;

		/* quick modulo, we know we're dealing with powers of 2 */
		/* there are twice as few input registers per bank, when
		 * iterating over all output registers we want to loop back to
		 * the start of the input when going over the halway mark */
		struct reg *l = &node->left[i & (count/2 - 1)];
		struct reg *r = &node->right[i & (count/2 - 1)];

		uint32_t el;
		uint32_t er;
		addr_fat_bfly(l->pkt.to, &el, NULL);
		addr_fat_bfly(r->pkt.to, &er, NULL);

		/* packet 'wants' to go right
		 * (wl = left packet wants to go right, could be better) */
		bool wl = (el >> height) & 1;
		bool wr = (er >> height) & 1;

		/* we accept a packet if the input register is busy and the
		 * packet wants to go in our direction */
		bool al = l->busy && (i < count / 2 ? !wl : wl);
		bool ar = r->busy && (i < count / 2 ? !wr : wr);

		/* can't accept either packet */
		if (!al && !ar)
			continue;

		/* select packet */
		struct reg *s = al ? l : r;
		if (al && ar)
			s = l->pkt.timestamp < r->pkt.timestamp ? l : r;

		node->out[i].pkt = s->pkt;
		node->out[i].busy = true;
		s->busy = false;
	}

	return OK;
}

static stat layer_clock(struct fat_node *layer, size_t count, size_t height)
{
	for (size_t i = 0; i < count; ++i)
		node_clock(&layer[i], 2ULL << height, height);

	return OK;
}

static stat fat_bfly_clock(struct fat_bfly *bfly)
{
	struct reg *out = bfly->layer[0]->out;
	for (size_t i = 0; i < bfly->elems; ++i) {
		if (!out[i].busy)
			continue;

		uint32_t elem;
		addr_fat_bfly(out[i].pkt.to, &elem, NULL);
		assert(elem == i);

		stat ret = SEND(bfly, bfly->send[i], out[i].pkt);
		if (ret == EBUSY)
			continue;

		out[i].busy = false;
	}

	for (size_t i = 0; i < bfly->layers; ++i)
		layer_clock(bfly->layer[i], 1ULL << i, bfly->layers - i - 1);

	for (size_t i = 0; i < bfly->elems; ++i) {
		if (!bfly->in[i].busy)
			continue;

		struct fat_node *node = &bfly->layer[bfly->layers - 1][i / 2];
		stat ret = node_receive(node, bfly->in[i].pkt, 0, i % 2);
		if (ret == EBUSY)
			continue;

		bfly->in[i].busy = false;
	}

	return OK;
}

static stat fat_bfly_receive(struct fat_bfly *bfly, struct component *from, struct packet pkt)
{
	uint32_t elem;
	addr_fat_bfly(pkt.from, &elem, NULL);
	assert(elem < bfly->elems);
	assert(bfly->send[elem] == from);

	if (bfly->in[elem].busy)
		return EBUSY;

	bfly->in[elem].pkt = pkt;
	bfly->in[elem].busy = true;
	return OK;
}

static void fat_bfly_destroy(struct fat_bfly *bfly)
{
	for (size_t i = 0; i < bfly->layers; ++i) {
		size_t count = 1ULL << i;
		for (size_t j = 0; j < count; ++j) {
			free(bfly->layer[i][j].out);
			free(bfly->layer[i][j].left);
			free(bfly->layer[i][j].right);
		}

		free(bfly->layer[i]);
	}

	free(bfly->in);
	free(bfly->send);
	free(bfly->layer);
	free(bfly);
}

struct component *create_fat_bfly(uint32_t elems)
{
	/* initialization of deeply nested pointer hierarchies is a bit of a
	 * pain, so just assert that allocations succeed for now. Might look
	 * into more graceful cleanup in the future. */
	assert(is_powerof2(elems));
	struct fat_bfly *bfly = calloc(1, sizeof(struct fat_bfly));
	assert(bfly);

	bfly->elems = elems;
	bfly->send = calloc(elems, sizeof(struct component *));
	assert(bfly->send);

	bfly->in = calloc(elems, sizeof(struct reg));
	assert(bfly->in);

	unsigned long long layers = log2ull(elems);

	bfly->layers = layers;
	bfly->layer = calloc(layers, sizeof(bfly->layers));
	assert(bfly->layers);

	for (size_t i = 0; i < layers; ++i) {
		size_t count = 1ULL << i;
		bfly->layer[i] = calloc(count, sizeof(struct fat_node));
		assert(bfly->layer[i]);

		for (size_t j = 0; j < count; ++j) {
			/* j / 2 since there are always twice as few nodes per layer */
			bfly->layer[i][j].up = i == 0 ? NULL : &bfly->layer[i - 1][j / 2];
			bfly->layer[i][j].left = calloc(elems / count, sizeof(struct reg));
			bfly->layer[i][j].right = calloc(elems / count, sizeof(struct reg));
			bfly->layer[i][j].out = calloc(2 * elems / count, sizeof(struct reg));
			assert(bfly->layer[i][j].left);
			assert(bfly->layer[i][j].right);
			assert(bfly->layer[i][j].out);
		}
	}

	bfly->component.clock = (clock_callback)fat_bfly_clock;
	bfly->component.receive = (receive_callback)fat_bfly_receive;
	bfly->component.destroy = (destroy_callback)fat_bfly_destroy;
	return (struct component *)bfly;
}

stat fat_bfly_connect(struct component *bfly, struct component *component, uint32_t elem)
{
	struct fat_bfly *b = (struct fat_bfly *)bfly;
	assert(elem < b->elems);
	assert(b->send[elem] == NULL);
	b->send[elem] = component;
	return OK;
}
