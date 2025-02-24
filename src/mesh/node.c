#include <gran/mesh/node.h>

struct reg {
	struct packet pkt;
	bool busy;
};

struct node {
	struct component component;
	uint16_t x, y;

	uint64_t timestamp;

	struct component *n, *s, *e, *w, *l;

	struct reg n_in, s_in, e_in, w_in, l_in;
};

enum order {
	N, S, E, W, L
};

static inline void maybe_pick(struct reg *output[5], enum order d, struct reg *r)
{
	if (output[d] && output[d]->pkt.timestamp < r->pkt.timestamp)
		return;

	output[d] = r;
}

static stat node_clock(struct node *node)
{
	node->timestamp++;

	struct reg *output[5] = {NULL, NULL, NULL, NULL, NULL};
	struct reg *input[5] = {
		&node->n_in,
		&node->s_in,
		&node->e_in,
		&node->w_in,
		&node->l_in
	};

	uint8_t X = node->x, Y = node->y;
	for (size_t i = 0; i < 5; ++i) {
		struct reg *r = input[i];
		if (!r->busy)
			continue;

		uint16_t x, y;
		addr_mesh(r->pkt.to, &x, &y, NULL);
		if (x < X) {
			maybe_pick(output, W, r);
			continue;
		}

		if (x > X) {
			maybe_pick(output, E, r);
			continue;
		}

		if (y < Y) {
			maybe_pick(output, S, r);
			continue;
		}

		if (y > Y) {
			maybe_pick(output, N, r);
			continue;
		}

		maybe_pick(output, L, r);
	}

	struct component *target[7] = {
		node->n,
		node->s,
		node->e,
		node->w,
		node->l
	};

	for (size_t i = 0; i < 5; ++i) {
		if (!output[i])
			continue;

		if (!target[i]) {
			/* for now, should send packet back with an error or something */
			abort();
		}

		stat ret = SEND(node, target[i], output[i]->pkt);
		if (ret == EBUSY)
			continue;

		assert(ret == OK);
		output[i]->busy = false;
	}

	return OK;
}

static stat reg_receive(struct reg *r, struct packet pkt)
{
	if (r->busy)
		return EBUSY;

	r->pkt = pkt;
	r->busy = true;
	return OK;
}

static stat node_receive(struct node *node, struct component *from, struct packet pkt)
{
	if (from == node->l) {
		/* add time when packet entered network */
		pkt.timestamp = node->timestamp;
		return reg_receive(&node->l_in, pkt);
	}

	if (from == node->n)
		return reg_receive(&node->n_in, pkt);

	if (from == node->s)
		return reg_receive(&node->s_in, pkt);

	if (from == node->e)
		return reg_receive(&node->e_in, pkt);

	if (from == node->w)
		return reg_receive(&node->w_in, pkt);

	abort();
	return OK;
}

struct component *create_mesh_node(uint16_t x, uint16_t y)
{
	struct node *node = calloc(1, sizeof(struct node));
	if (!node)
		return NULL;

	node->component.receive = (receive_callback)node_receive;
	node->component.clock = (clock_callback)node_clock;
	node->x = x;
	node->y = y;
	return (struct component *)node;
}

stat mesh_node_connect(struct component *node,
		struct component *n,
		struct component *s,
		struct component *e,
		struct component *w,
		struct component *l)
{
	struct node *nod = (struct node *)node;
	nod->n = n;
	nod->s = s;
	nod->e = e;
	nod->w = w;
	nod->l = l;
	return OK;
}
