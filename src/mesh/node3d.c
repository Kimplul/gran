#include <gran/mesh/node3d.h>

#define north_port(n) (n)->ports[(n)->elems + 0]
#define east_port(n) (n)->ports[(n)->elems + 1]
#define south_port(n) (n)->ports[(n)->elems + 2]
#define west_port(n) (n)->ports[(n)->elems + 3]
#define up_port(n) (n)->ports[(n)->elems + 4]
#define down_port(n) (n)->ports[(n)->elems + 5]

#define north_in(n) (n)->in[(n)->elems + 0]
#define east_in(n) (n)->in[(n)->elems + 1]
#define south_in(n) (n)->in[(n)->elems + 2]
#define west_in(n) (n)->in[(n)->elems + 3]
#define up_in(n) (n)->in[(n)->elems + 4]
#define down_in(n) (n)->in[(n)->elems + 5]

#define north_out(n) (n)->out[(n)->elems + 0]
#define east_out(n) (n)->out[(n)->elems + 1]
#define south_out(n) (n)->out[(n)->elems + 2]
#define west_out(n) (n)->out[(n)->elems + 3]
#define up_out(n) (n)->out[(n)->elems + 4]
#define down_out(n) (n)->out[(n)->elems + 5]

struct node3d {
	struct component component;
	uint8_t x, y, z;
	uint8_t elems;

	uint64_t timestamp;

	struct reg *in; /* countedby[elems + 6] */
	struct reg *out; /* countedby[elems + 6] */
	struct component **ports; /* countedby[elems + 6] */
};

static void node3d_destroy(struct node3d *n)
{
	free(n->in);
	free(n->out);
	free(n->ports);
	free(n);
}

static void clock_outputs(struct node3d *n)
{
	for (int i = 0; i < n->elems + 6; ++i) {
		if (!n->out[i].busy)
			continue;

		stat ret = SEND(n, n->ports[i], n->out[i].pkt);
		if (ret == EBUSY)
			continue;

		n->out[i].busy = false;
	}
}

struct sel_helper {
	uint8_t x, y, z, elem;
};

static bool north_sel(struct reg *r, void *data)
{
	uint8_t y = 0;
	struct sel_helper *helper = data;
	addr_mesh3d(r->pkt.to, NULL, &y, NULL, NULL, NULL);
	return y > helper->y;
}

static bool south_sel(struct reg *r, void *data)
{
	uint8_t y = 0;
	struct sel_helper *helper = data;
	addr_mesh3d(r->pkt.to, NULL, &y, NULL, NULL, NULL);
	return y < helper->y;
}

static bool east_sel(struct reg *r, void *data)
{
	uint8_t x = 0, y = 0;
	struct sel_helper *helper = data;
	addr_mesh3d(r->pkt.to, &x, &y, NULL, NULL, NULL);
	return y == helper->y && x > helper->x;
}

static bool west_sel(struct reg *r, void *data)
{
	uint8_t x = 0, y = 0;
	struct sel_helper *helper = data;
	addr_mesh3d(r->pkt.to, &x, &y, NULL, NULL, NULL);
	return y == helper->y && x < helper->x;
}

static bool up_sel(struct reg *r, void *data)
{
	uint8_t x = 0, y = 0, z = 0;
	struct sel_helper *helper = data;
	addr_mesh3d(r->pkt.to, &x, &y, &z, NULL, NULL);
	return y == helper->y && x == helper->x && z > helper->z;
}

static bool down_sel(struct reg *r, void *data)
{
	uint8_t x = 0, y = 0, z = 0;
	struct sel_helper *helper = data;
	addr_mesh3d(r->pkt.to, &x, &y, &z, NULL, NULL);
	return y == helper->y && x == helper->x && z < helper->z;
}

static bool elem_sel(struct reg *r, void *data)
{
	uint8_t x = 0, y = 0, z = 0, elem = 0;
	struct sel_helper *helper = data;
	addr_mesh3d(r->pkt.to, &x, &y, &z, &elem, NULL);
	return x == helper->x && y == helper->y && z == helper->z &&
	       elem == helper->elem;
}

static stat node3d_clock(struct node3d *n)
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
	};

	struct sel_helper helper = {
		.elem = 0,
		.x = n->x,
		.y = n->y,
		.z = n->z
	};

	struct reg *north[] = {r,            &east_in(n),  &south_in(n),
		               &west_in(n),  &up_in(n),    &down_in(n)};

	struct reg *east[]  = {r,            &north_in(n), &south_in(n),
		               &west_in(n),  &up_in(n),    &down_in(n)};

	struct reg *south[] = {r,            &north_in(n), &east_in(n),
		               &west_in(n),  &up_in(n),    &down_in(n)};

	struct reg *west[]  = {r,            &north_in(n), &east_in(n),
		               &south_in(n), &up_in(n),    &down_in(n)};

	struct reg *up[]    = {r,            &north_in(n), &east_in(n),
		               &west_in(n),  &south_in(n), &down_in(n)};

	struct reg *down[]  = {r,            &north_in(n), &east_in(n),
		               &west_in(n),  &south_in(n), &up_in(n)};

	propagate(&north_out(n), 6, north, north_sel, &helper);
	propagate(&east_out(n),  6, east,  east_sel,  &helper);
	propagate(&south_out(n), 6, south, south_sel, &helper);
	propagate(&west_out(n),  6, west,  west_sel,  &helper);
	propagate(&up_out(n),    6, up,    up_sel,    &helper);
	propagate(&down_out(n),  6, down,  down_sel,  &helper);

	struct reg *all[] = {r, &north_in(n), &east_in(n), &south_in(n),
		             &west_in(n),  &up_in(n),   &down_in(n)};

	for (int i = 0; i < n->elems; ++i) {
		helper.elem = i;
		propagate(&n->out[i], 7, all, elem_sel, &helper);
	}

	return OK;
}

static stat node3d_receive(struct node3d *n, struct component *from,
                           struct packet pkt)
{
	for (int i = 0; i < n->elems + 6; ++i) {
		if (from != n->ports[i])
			continue;

		if (i < n->elems)
			pkt.timestamp = n->timestamp;

		return place_reg(&n->in[i], pkt);
	}

	abort();
	return OK;
}

struct component *create_mesh_node3d(uint8_t x, uint8_t y, uint8_t z,
                                     uint8_t elems)
{
	struct node3d *n = calloc(1, sizeof(struct node3d));
	if (!n)
		return NULL;

	n->in = calloc(elems + 6, sizeof(struct reg));
	if (!n->in) {
		node3d_destroy(n);
		return NULL;
	}

	n->out = calloc(elems + 6, sizeof(struct reg));
	if (!n->out) {
		node3d_destroy(n);
		return NULL;
	}

	n->ports = calloc(elems + 6, sizeof(struct component*));
	if (!n->ports) {
		node3d_destroy(n);
		return NULL;
	}

	n->component.destroy = (destroy_callback)node3d_destroy;
	n->component.receive = (receive_callback)node3d_receive;
	n->component.clock = (clock_callback)node3d_clock;
	n->elems = elems;
	n->x = x;
	n->y = y;
	n->z = z;
	return (struct component *)n;
}

stat mesh_node3d_connect(struct component *c, struct component *e,
                         uint8_t elem)
{
	struct node3d *n = (struct node3d *)c;
	if (elem >= n->elems)
		return ENOSUCH;

	if (n->ports[elem])
		return EEXISTS;

	n->ports[elem] = e;
	return OK;
}

stat mesh_node3d_connect_north(struct component *c, struct component *e)
{
	struct node3d *n = (struct node3d *)c;
	if (north_port(n))
		return EEXISTS;

	north_port(n) = e;
	return OK;
}

stat mesh_node3d_connect_east(struct component *c, struct component *e)
{
	struct node3d *n = (struct node3d *)c;
	if (east_port(n))
		return EEXISTS;

	east_port(n) = e;
	return OK;
}

stat mesh_node3d_connect_south(struct component *c, struct component *e)
{
	struct node3d *n = (struct node3d *)c;
	if (south_port(n))
		return EEXISTS;

	south_port(n) = e;
	return OK;
}

stat mesh_node3d_connect_west(struct component *c, struct component *e)
{
	struct node3d *n = (struct node3d *)c;
	if (west_port(n))
		return EEXISTS;

	west_port(n) = e;
	return OK;
}

stat mesh_node3d_connect_up(struct component *c, struct component *e)
{
	struct node3d *n = (struct node3d *)c;
	if (up_port(n))
		return EEXISTS;

	up_port(n) = e;
	return OK;
}

stat mesh_node3d_connect_down(struct component *c, struct component *e)
{
	struct node3d *n = (struct node3d *)c;
	if (down_port(n))
		return EEXISTS;

	down_port(n) = e;
	return OK;
}
