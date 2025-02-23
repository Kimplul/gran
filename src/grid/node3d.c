#include <gran/grid/node3d.h>

struct reg {
	struct packet pkt;
	bool busy;
};

struct node3d {
	struct component component;
	uint8_t x, y, z;

	uint64_t timestamp;

	struct component *n, *s, *w, *e, *u, *d, *l;

	struct reg n_in, s_in, w_in, e_in, u_in, d_in, l_in;
};

static stat reg_receive(struct reg *r, struct packet pkt)
{
	if (r->busy)
		return EBUSY;

	r->pkt = pkt;
	r->busy = true;
	return OK;
}

/* LX = node.x > pkt.x, etc */
enum match {
	LX = (1 << 0),
	LY = (1 << 1),
	LZ = (1 << 2),
	GX = (1 << 3),
	GY = (1 << 4),
	GZ = (1 << 5),
	NX = (1 << 6),
	NY = (1 << 7),
	NZ = (1 << 8),
};

static void maybe_route_reg(struct node3d *node3d, struct reg *reg, enum match m, uint64_t *oldest, struct packet **pkt, bool **busy)
{
	if (!reg->busy)
		return;

	uint8_t x, y, z;
	addr_grid3d(reg->pkt.to, &x, &y, &z, NULL);

	if ((m & LX) && x < node3d->x)
		return;

	if ((m & LY) && y < node3d->y)
		return;

	if ((m & LZ) && z < node3d->z)
		return;

	if ((m & GX) && x > node3d->x)
		return;

	if ((m & GY) && y > node3d->y)
		return;

	if ((m & GZ) && z > node3d->z)
		return;

	if ((m & NX) && x == node3d->x)
		return;

	if ((m & NY) && y == node3d->y)
		return;

	if ((m & NZ) && z == node3d->z)
		return;

	if (reg->pkt.timestamp < *oldest) {
		*oldest = reg->pkt.timestamp;
		*busy = &reg->busy;
		*pkt = &reg->pkt;
	}
}

static stat route(struct node3d *node3d, struct component *next, enum match m)
{
	uint64_t oldest = -1; struct packet *pkt = NULL; bool *busy = NULL;
	maybe_route_reg(node3d, &node3d->l_in, m, &oldest, &pkt, &busy);
	maybe_route_reg(node3d, &node3d->n_in, m, &oldest, &pkt, &busy);
	maybe_route_reg(node3d, &node3d->s_in, m, &oldest, &pkt, &busy);
	maybe_route_reg(node3d, &node3d->e_in, m, &oldest, &pkt, &busy);
	maybe_route_reg(node3d, &node3d->w_in, m, &oldest, &pkt, &busy);
	maybe_route_reg(node3d, &node3d->u_in, m, &oldest, &pkt, &busy);
	maybe_route_reg(node3d, &node3d->d_in, m, &oldest, &pkt, &busy);

	/* no suitable match */
	if (pkt == NULL)
		return OK;

	assert(busy);

	if (!next) {
		abort(); /* for now, eventually should probably return to sender */
		return OK;
	}

	stat ret = SEND(node3d, next, *pkt);
	if (ret == EBUSY)
		return OK;

	*busy = false;
	return OK;
}

static stat node3d_clock(struct node3d *node3d)
{
	node3d->timestamp++;

	stat ret = OK;
	if ((ret = route(node3d, node3d->e, LX | NX)))
		return ret;

	if ((ret = route(node3d, node3d->w, GX | NX)))
		return ret;

	if ((ret = route(node3d, node3d->n, LX | GX | LY | NY)))
		return ret;

	if ((ret = route(node3d, node3d->s, LX | GX | GY | NY)))
		return ret;

	if ((ret = route(node3d, node3d->u, LX | GX | LY | GY | LZ | NZ)))
		return ret;

	if ((ret = route(node3d, node3d->d, LX | GX | LY | GY | GZ | NZ)))
		return ret;

	if ((ret = route(node3d, node3d->l, LX | LY | LZ | GX | GY | GZ)))
		return ret;

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

	if (from == node3d->w)
		return reg_receive(&node3d->w_in, pkt);

	if (from == node3d->e)
		return reg_receive(&node3d->e_in, pkt);

	if (from == node3d->u)
		return reg_receive(&node3d->u_in, pkt);

	if (from == node3d->d)
		return reg_receive(&node3d->d_in, pkt);

	abort();
	return OK;
}

struct component *create_grid_node3d(uint8_t x, uint8_t y, uint8_t z)
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

stat grid_node3d_connect(struct component *node,
		struct component *n,
		struct component *s,
		struct component *w,
		struct component *e,
		struct component *u,
		struct component *d,
		struct component *l)
{
	struct node3d *node3d = (struct node3d *)node;
	node3d->n = n;
	node3d->s = s;
	node3d->w = w;
	node3d->e = e;
	node3d->u = u;
	node3d->d = d;
	node3d->l = l;
	return OK;
}
