#include <gran/torus3d/node.h>

struct port {
	struct reg r[2];
};

struct torus3d_node {
	struct component component;
	uint8_t x, y, z /*, w for 4D but that might be a bit overkill*/;

	struct component *x_next, *y_next, *z_next,
	        *x_prev, *y_prev, *z_prev,
	        *child;

	struct port port_x, port_y, port_z;

	struct reg x_in, y_in, z_in, child_in;

	bool prio;
};

static stat port_receive(struct torus3d_node *torus3d, struct port *port,
                         struct reg *reg)
{
	if (!reg->busy)
		return OK;

	/* source */
	uint8_t sx, sy, sz;
	addr_torus3d(reg->pkt.from, &sx, &sy, &sz, NULL);

	/* dst */
	uint8_t dx, dy, dz;
	addr_torus3d(reg->pkt.to, &dx, &dy, &dz, NULL);

	/* Dally/spiral routing though I'm a bit unsure if this works for 2D/3D
	 * toruses (1D seems to work, 2D not so much atm) */
	int chan = is_set(&reg->pkt, PACKET_DONE);
	if (port->r[chan].busy)
		return OK;

	printf("(%d, %d, %d) to (%d, %d, %d) via (%d, %d, %d) c %d\n",
	       sx, sy, sz,
	       dx, dy, dz,
	       torus3d->x, torus3d->y, torus3d->z,
	       chan);

	port->r[chan].pkt = reg->pkt;
	port->r[chan].busy = true;
	reg->busy = false;
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

enum match {
	MX = (1 << 0),
	MY = (1 << 1),
	MZ = (1 << 2),
	NX = (1 << 3),
	NY = (1 << 4),
	NZ = (1 << 5),
};

static void maybe_route_reg(struct torus3d_node *torus3d, struct reg *reg,
                            enum match m, uint64_t *oldest, struct packet **pkt,
                            bool **busy)
{
	if (!reg->busy)
		return;

	uint8_t x, y, z;
	addr_torus3d(reg->pkt.to, &x, &y, &z, NULL);

	if ((m & MX) && x != torus3d->x)
		return;

	if ((m & MY) && y != torus3d->y)
		return;

	if ((m & MZ) && z != torus3d->z)
		return;

	if ((m & NX) && x == torus3d->x)
		return;

	if ((m & NY) && y == torus3d->y)
		return;

	if ((m & NZ) && z == torus3d->z)
		return;

	if (reg->pkt.timestamp < *oldest) {
		*oldest = reg->pkt.timestamp;
		*busy = &reg->busy;
		*pkt = &reg->pkt;
	}
}

static stat route(struct torus3d_node *torus3d, struct component *next,
                  enum match m)
{
	bool prio = torus3d->prio;
	uint64_t oldest = -1; struct packet *pkt = NULL; bool *busy = NULL;

	/* prioritise current priority port */
	maybe_route_reg(torus3d, &torus3d->port_x.r[prio], m,
			&oldest, &pkt, &busy);

	maybe_route_reg(torus3d, &torus3d->port_y.r[prio], m,
			&oldest, &pkt, &busy);

	maybe_route_reg(torus3d, &torus3d->port_z.r[prio], m,
			&oldest, &pkt, &busy);

	/* if no suitable match found, check other ports as well */
	if (pkt == NULL) {
		maybe_route_reg(torus3d, &torus3d->port_x.r[!prio], m,
				&oldest, &pkt, &busy);

		maybe_route_reg(torus3d, &torus3d->port_y.r[!prio], m,
				&oldest, &pkt, &busy);

		maybe_route_reg(torus3d, &torus3d->port_z.r[!prio], m,
				&oldest, &pkt, &busy);
	}

	maybe_route_reg(torus3d, &torus3d->child_in, m,
			&oldest, &pkt, &busy);

	/* no suitable match */
	if (pkt == NULL)
		return OK;

	assert(busy);

	stat ret = SEND(torus3d, next, *pkt);
	if (ret == EBUSY)
		return OK;

	*busy = false;
	return OK;
}

static stat torus3d_clock(struct torus3d_node *torus3d)
{
	torus3d->prio = !torus3d->prio;

	stat ret = OK;
	if ((ret = port_receive(torus3d, &torus3d->port_x, &torus3d->x_in)))
		return ret;

	if ((ret = port_receive(torus3d, &torus3d->port_y, &torus3d->y_in)))
		return ret;

	if ((ret = port_receive(torus3d, &torus3d->port_z, &torus3d->z_in)))
		return ret;

	if ((ret = route(torus3d, torus3d->x_next, NX)))
		return ret;

	if ((ret = route(torus3d, torus3d->y_next, NY | MX)))
		return ret;

	if ((ret = route(torus3d, torus3d->z_next, NZ | MX | MY)))
		return ret;

	if ((ret = route(torus3d, torus3d->child, MX | MY | MZ)))
		return ret;

	return OK;
}

static stat torus3d_receive(struct torus3d_node *torus3d,
                            struct component *from, struct packet pkt)
{
	if (from == torus3d->child)
		return reg_receive(&torus3d->child_in, pkt);

	if (from == torus3d->x_prev)
		return reg_receive(&torus3d->x_in, pkt);

	if (from == torus3d->y_prev)
		return reg_receive(&torus3d->y_in, pkt);

	if (from == torus3d->z_prev)
		return reg_receive(&torus3d->z_in, pkt);

	abort();
	return OK;
}

struct component *create_torus3d_node(uint8_t x, uint8_t y, uint8_t z)
{
	struct torus3d_node *node = calloc(1, sizeof(struct torus3d_node));
	if (!node)
		return NULL;

	node->component.receive = (receive_callback)torus3d_receive;
	node->component.clock = (clock_callback)torus3d_clock;
	node->x = x;
	node->y = y;
	node->z = z;
	return (struct component *)node;
}

stat torus3d_node_connect(struct component *node,
                          struct component *x_in,
                          struct component *y_in,
                          struct component *z_in,
                          struct component *child,
                          struct component *x_out,
                          struct component *y_out,
                          struct component *z_out)
{
	struct torus3d_node *n = (struct torus3d_node *)node;
	n->x_prev = x_in;
	n->y_prev = y_in;
	n->z_prev = z_in;
	n->child = child;
	n->x_next = x_out;
	n->y_next = y_out;
	n->z_next = z_out;
	return OK;
}
