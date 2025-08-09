#include <gran/mesh/node1d.h>

#define north_port(n) (n)->ports[(n)->elems + 0]
#define south_port(n) (n)->ports[(n)->elems + 1]

#define north_in(n) (n)->in[(n)->elems + 0]
#define south_in(n) (n)->in[(n)->elems + 1]

#define north_out(n) (n)->out[(n)->elems + 0]
#define south_out(n) (n)->out[(n)->elems + 1]

struct node1d {
	struct component component;
	uint16_t cluster;
	uint16_t elems;

	uint64_t timestamp;

	struct reg *in; /* countedby[elems + 2] */
	struct reg *out; /* countedby[elems + 2] */
	struct component **ports; /* countedby[elems + 2] */
};

static void node1d_destroy(struct node1d *n)
{
	free(n->in);
	free(n->out);
	free(n->ports);
	free(n);
}

static stat node1d_receive(struct node1d *n, struct component *from,
                           struct packet pkt)
{
	for (int i = 0; i < n->elems + 2; ++i) {
		/* add timestamp to packets that originate with us */
		if (i < n->elems)
			pkt.timestamp = n->timestamp;

		if (from == n->ports[i])
			return place_reg(&n->in[i], pkt);
	}

	/* shouldn't be possible */
	abort();
	return OK;
}

static void clock_outputs(struct node1d *n)
{
	for (int i = 0; i < n->elems + 2; ++i) {
		if (!n->out[i].busy)
			continue;

		stat ret = SEND(n, n->ports[i], n->out[i].pkt);
		if (ret == EBUSY)
			continue;

		n->out[i].busy = false;
	}
}

struct sel_helper {
	uint16_t cluster, elem;
};

static bool north_sel(struct reg *r, void *data)
{
	uint16_t cluster = 0;
	struct sel_helper *helper = data;
	addr_mesh1d(r->pkt.to, &cluster, NULL, NULL);
	return cluster > helper->cluster;
}

static bool south_sel(struct reg *r, void *data)
{
	uint16_t cluster = 0;
	struct sel_helper *helper = data;
	addr_mesh1d(r->pkt.to, &cluster, NULL, NULL);
	return cluster < helper->cluster;
}

static bool elem_sel(struct reg *r, void *data)
{
	uint16_t cluster = 0, elem = 0;
	struct sel_helper *helper = data;
	addr_mesh1d(r->pkt.to, &cluster, &elem, NULL);
	return cluster == helper->cluster && elem == helper->elem;
}

static stat node1d_clock(struct node1d *n)
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
		.cluster = n->cluster,
		.elem = 0,
	};

	struct reg *north[] = {r, &south_in(n)};
	struct reg *south[] = {r, &north_in(n)};

	propagate(&north_out(n), 2, north, north_sel, &helper);
	propagate(&south_out(n), 2, south, south_sel, &helper);

	struct reg *all[] = {r, &north_in(n), &south_in(n)};
	for (int i = 0; i < n->elems; ++i) {
		helper.elem = i;
		propagate(&n->out[i], 3, all, elem_sel, &helper);
	}

	return OK;
}

stat mesh_node1d_connect(struct component *c, struct component *e,
                         uint16_t elem)
{
	struct node1d *n = (struct node1d *)c;
	if (elem >= n->elems)
		return ENOSUCH;

	if (n->ports[elem])
		return EEXISTS;

	n->ports[elem] = e;
	return OK;
}

stat mesh_node1d_connect_north(struct component *c, struct component *e)
{
	struct node1d *n = (struct node1d *)c;
	if (north_port(n))
		return EEXISTS;

	north_port(n) = e;
	return OK;
}

stat mesh_node1d_connect_south(struct component *c, struct component *e)
{
	struct node1d *n = (struct node1d *)c;
	if (south_port(n))
		return EEXISTS;

	south_port(n) = e;
	return OK;
}

struct component *create_mesh_node1d(uint16_t cluster, uint16_t elems)
{
	struct node1d *n = (struct node1d *)calloc(1, sizeof(struct node1d));
	if (!n)
		return NULL;

	n->in = (struct reg *)calloc(elems + 2, sizeof(struct reg));
	if (!n->in) {
		node1d_destroy(n);
		return NULL;
	}

	n->out = (struct reg *)calloc(elems + 2, sizeof(struct reg));
	if (!n->out) {
		node1d_destroy(n);
		return NULL;
	}

	n->ports = (struct component **)calloc(elems + 2,
	                                       sizeof(struct component *));
	if (!n->ports) {
		node1d_destroy(n);
		return NULL;
	}

	n->component.destroy = (destroy_callback)node1d_destroy;
	n->component.receive = (receive_callback)node1d_receive;
	n->component.clock   = (clock_callback)node1d_clock;
	n->cluster = cluster;
	n->elems = elems;
	return (struct component *)n;
}
