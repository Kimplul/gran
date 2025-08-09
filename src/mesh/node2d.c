#include <gran/mesh/node2d.h>

#define north_port(n) (n)->ports[(n)->elems + 0]
#define east_port(n) (n)->ports[(n)->elems + 1]
#define south_port(n) (n)->ports[(n)->elems + 2]
#define west_port(n) (n)->ports[(n)->elems + 3]

#define north_in(n) (n)->in[(n)->elems + 0]
#define east_in(n) (n)->in[(n)->elems + 1]
#define south_in(n) (n)->in[(n)->elems + 2]
#define west_in(n) (n)->in[(n)->elems + 3]

#define north_out(n) (n)->out[(n)->elems + 0]
#define east_out(n) (n)->out[(n)->elems + 1]
#define south_out(n) (n)->out[(n)->elems + 2]
#define west_out(n) (n)->out[(n)->elems + 3]

struct node2d {
	struct component component;
	uint16_t elems;
	uint16_t x, y;

	uint64_t timestamp;

	struct reg *in; /* countedby[elems + 4] */
	struct reg *out; /* countedby[elems + 4] */
	struct component **ports; /* countedby[elems + 4] */
};

static void node2d_destroy(struct node2d *n)
{
	free(n->in);
	free(n->out);
	free(n->ports);
	free(n);
}

static void clock_outputs(struct node2d *n)
{
	for (int i = 0; i < n->elems + 4; ++i) {
		if (!n->out[i].busy)
			continue;

		stat ret = SEND(n, n->ports[i], n->out[i].pkt);
		if (ret == EBUSY)
			continue;

		n->out[i].busy = false;
	}
}

struct sel_helper {
	uint8_t x, y;
	uint16_t elem;
};

static bool north_sel(struct reg *r, void *data)
{
	uint8_t y = 0;
	struct sel_helper *helper = data;
	addr_mesh2d(r->pkt.to, NULL, &y, NULL, NULL);
	return y > helper->y;
}

static bool south_sel(struct reg *r, void *data)
{
	uint8_t y = 0;
	struct sel_helper *helper = data;
	addr_mesh2d(r->pkt.to, NULL, &y, NULL, NULL);
	return y < helper->y;
}

static bool east_sel(struct reg *r, void *data)
{
	uint8_t x = 0, y = 0;
	struct sel_helper *helper = data;
	addr_mesh2d(r->pkt.to, &x, &y, NULL, NULL);
	return y == helper->y && x > helper->x;
}

static bool west_sel(struct reg *r, void *data)
{
	uint8_t x = 0, y = 0;
	struct sel_helper *helper = data;
	addr_mesh2d(r->pkt.to, &x, &y, NULL, NULL);
	return y == helper->y && x < helper->x;
}

static bool elem_sel(struct reg *r, void *data)
{
	uint16_t elem = 0;
	uint8_t x = 0, y = 0;
	struct sel_helper *helper = data;
	addr_mesh2d(r->pkt.to, &x, &y, &elem, NULL);
	return x == helper->x && y == helper->y && elem == helper->elem;
}

static stat node2d_clock(struct node2d *n)
{
	n->timestamp++;
	clock_outputs(n);

	/* select oldest packet to process */
	struct reg *r = NULL;
	for (int i = 0; i < n->elems; ++i) {
		if (!n->in[i].busy)
			continue;

		if (!r || r->pkt.timestamp > n->in[i].pkt.timestamp)
			r = &n->in[i];
	}

	struct sel_helper helper = {
		.elem = 0,
		.x = n->x,
		.y = n->y,
	};

	struct reg *north[] = {r, &east_in(n),  &south_in(n), &west_in(n)};
	struct reg *east[]  = {r, &north_in(n), &south_in(n), &west_in(n)};
	struct reg *south[] = {r, &north_in(n), &east_in(n),  &west_in(n)};
	struct reg *west[]  = {r, &north_in(n), &east_in(n),  &south_in(n)};

	propagate(&north_out(n), 4, north, north_sel, &helper);
	propagate(&east_out(n),  4, east,  east_sel,  &helper);
	propagate(&south_out(n), 4, south, south_sel, &helper);
	propagate(&west_out(n),  4, west,  west_sel,  &helper);

	struct reg *all[] = {r, &north_in(n), &east_in(n), &south_in(n), &west_in(n)};
	for (int i = 0; i < n->elems; ++i) {
		helper.elem = i;
		propagate(&n->out[i], 5, all, elem_sel, &helper);
	}

	return OK;
}

static stat node2d_receive(struct node2d *n, struct component *from,
                           struct packet pkt)
{
	for (int i = 0; i < n->elems + 4; ++i) {
		if (from != n->ports[i])
			continue;

		if (i < n->elems)
			pkt.timestamp = n->timestamp;

		return place_reg(&n->in[i], pkt);
	}

	abort();
	return OK;
}

struct component *create_mesh_node2d(uint8_t x, uint8_t y, uint16_t elems)
{
	struct node2d *n = calloc(1, sizeof(struct node2d));
	if (!n)
		return NULL;

	n->in = calloc(elems + 4, sizeof(struct reg));
	if (!n->in) {
		node2d_destroy(n);
		return NULL;
	}

	n->out = calloc(elems + 4, sizeof(struct reg));
	if (!n->out) {
		node2d_destroy(n);
		return NULL;
	}

	n->ports = calloc(elems + 4, sizeof(struct component *));
	if (!n->ports) {
		node2d_destroy(n);
		return NULL;
	}

	n->component.destroy = (destroy_callback)node2d_destroy;
	n->component.receive = (receive_callback)node2d_receive;
	n->component.clock = (clock_callback)node2d_clock;
	n->elems = elems;
	n->x = x;
	n->y = y;
	return (struct component *)n;
}

stat mesh_node2d_connect(struct component *c, struct component *e,
                         uint16_t elem)
{
	struct node2d *n = (struct node2d *)c;
	if (elem >= n->elems)
		return ENOSUCH;

	if (n->ports[elem])
		return EEXISTS;

	n->ports[elem] = e;
	return OK;
}

stat mesh_node2d_connect_north(struct component *c, struct component *e)
{
	struct node2d *n = (struct node2d *)c;
	if (north_port(n))
		return EEXISTS;

	north_port(n) = e;
	return OK;
}

stat mesh_node2d_connect_east(struct component *c, struct component *e)
{
	struct node2d *n = (struct node2d *)c;
	if (east_port(n))
		return EEXISTS;

	east_port(n) = e;
	return OK;
}

stat mesh_node2d_connect_south(struct component *c, struct component *e)
{
	struct node2d *n = (struct node2d *)c;
	if (south_port(n))
		return EEXISTS;

	south_port(n) = e;
	return OK;
}

stat mesh_node2d_connect_west(struct component *c, struct component *e)
{
	struct node2d *n = (struct node2d *)c;
	if (west_port(n))
		return EEXISTS;

	west_port(n) = e;
	return OK;
}
