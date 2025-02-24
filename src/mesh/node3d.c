#include <gran/mesh/node3d.h>

struct reg {
	struct packet pkt;
	bool busy;
};

struct node3d {
	struct component component;
	uint8_t x, y, z;

	uint64_t timestamp;

	struct component *n, *s, *e, *w, *u, *d, *l;

	struct reg n_in, s_in, e_in, w_in, u_in, d_in, l_in;
};

enum order {
	N, S, E, W, U, D, L
};

static inline void maybe_pick(struct reg *output[7], enum order d, struct reg *r)
{
	if (output[d] && output[d]->pkt.timestamp < r->pkt.timestamp)
		return;

	output[d] = r;
}

static stat node3d_clock(struct node3d *node3d)
{
	node3d->timestamp++;

	struct reg *output[7] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	struct reg *input[7] = {
		&node3d->n_in,
		&node3d->s_in,
		&node3d->e_in,
		&node3d->w_in,
		&node3d->u_in,
		&node3d->d_in,
		&node3d->l_in
	};

	uint8_t X = node3d->x, Y = node3d->y, Z = node3d->z;
	for (size_t i = 0; i < 7; ++i) {
		struct reg *r = input[i];
		if (!r->busy)
			continue;

		uint8_t x, y, z;
		addr_mesh3d(r->pkt.to, &x, &y, &z, NULL);
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

		if (z < Z) {
			maybe_pick(output, D, r);
			continue;
		}

		if (z > Z) {
			maybe_pick(output, U, r);
			continue;
		}

		maybe_pick(output, L, r);
	}

	struct component *target[7] = {
		node3d->n,
		node3d->s,
		node3d->e,
		node3d->w,
		node3d->u,
		node3d->d,
		node3d->l
	};

	for (size_t i = 0; i < 7; ++i) {
		if (!output[i])
			continue;

		if (!target[i]) {
			/* for now, should send packet back with an error or something */
			abort();
		}

		stat ret = SEND(node3d, target[i], output[i]->pkt);
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

static stat node3d_receive(struct node3d *node3d, struct component *from, struct packet pkt)
{
	if (from == node3d->l) {
		/* add time when packet entered network */
		pkt.timestamp = node3d->timestamp;
		return reg_receive(&node3d->l_in, pkt);
	}

	if (from == node3d->n)
		return reg_receive(&node3d->n_in, pkt);

	if (from == node3d->s)
		return reg_receive(&node3d->s_in, pkt);

	if (from == node3d->e)
		return reg_receive(&node3d->e_in, pkt);

	if (from == node3d->w)
		return reg_receive(&node3d->w_in, pkt);

	if (from == node3d->u)
		return reg_receive(&node3d->u_in, pkt);

	if (from == node3d->d)
		return reg_receive(&node3d->d_in, pkt);

	abort();
	return OK;
}

struct component *create_mesh_node3d(uint8_t x, uint8_t y, uint8_t z)
{
	struct node3d *node = calloc(1, sizeof(struct node3d));
	if (!node)
		return NULL;

	node->component.receive = (receive_callback)node3d_receive;
	node->component.clock = (clock_callback)node3d_clock;
	node->x = x;
	node->y = y;
	node->z = z;
	return (struct component *)node;
}

stat mesh_node3d_connect(struct component *node,
		struct component *n,
		struct component *s,
		struct component *e,
		struct component *w,
		struct component *u,
		struct component *d,
		struct component *l)
{
	struct node3d *node3d = (struct node3d *)node;
	node3d->n = n;
	node3d->s = s;
	node3d->e = e;
	node3d->w = w;
	node3d->u = u;
	node3d->d = d;
	node3d->l = l;
	return OK;
}
