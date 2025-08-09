#include <gran/mesh/node1d.h>

#define left_port(n) (n)->ports[(n)->elems + 0]
#define right_port(n) (n)->ports[(n)->elems + 1]

#define left_in(n) (n)->in[(n)->elems + 0]
#define right_in(n) (n)->in[(n)->elems + 1]

#define left_out(n) (n)->out[(n)->elems + 0]
#define right_out(n) (n)->out[(n)->elems + 1]

struct reg {
	struct packet pkt;
	bool busy;
};

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
}

static stat reg_busy(struct reg *r, struct packet pkt)
{
	bool busy = r->busy;
	if (!busy) {
		r->pkt = pkt;
		r->busy = true;
	}

	return busy ? EBUSY : OK;
}

static void copy_reg(struct reg *r, struct reg *s)
{
	assert(s->busy);
	if (r->busy)
		return;

	r->pkt = s->pkt;
	r->busy = true;
	s->busy = false;
}

static stat node1d_receive(struct node1d *n, struct component *from, struct packet pkt)
{
	for (int i = 0; i < n->elems + 2; ++i) {
		/* add timestamp to packets that originate with us */
		if (i < n->elems)
			pkt.timestamp = n->timestamp;

		if (from == n->ports[i])
			return reg_busy(&n->in[i], pkt);
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

static void propagate_left(struct node1d *n, struct reg *a, struct reg *b)
{
	struct reg *sel_a = NULL, *sel_b = NULL;
	if (a && a->busy) {
		uint16_t cluster = 0;
		addr_mesh1d(a->pkt.to, &cluster, NULL, NULL);

		if (cluster > n->cluster)
			sel_a = a;
	}

	if (b && b->busy) {
		uint16_t cluster = 0;
		addr_mesh1d(b->pkt.to, &cluster, NULL, NULL);

		if (cluster > n->cluster)
			sel_b = b;
	}

	if (!sel_a && !sel_b)
		return;

	if (sel_a && !sel_b) {
		copy_reg(&left_out(n), sel_a);
		return;
	}

	if (!sel_a && sel_b) {
		copy_reg(&left_out(n), sel_b);
		return;
	}

	/* both available, select older */
	if (sel_a->pkt.timestamp < sel_b->pkt.timestamp)
		copy_reg(&left_out(n), sel_a);
	else
		copy_reg(&left_out(n), sel_b);
}

static void propagate_right(struct node1d *n, struct reg *a, struct reg *b)
{
	struct reg *sel_a = NULL, *sel_b = NULL;
	if (a && a->busy) {
		uint16_t cluster = 0;
		addr_mesh1d(a->pkt.to, &cluster, NULL, NULL);

		if (cluster < n->cluster)
			sel_a = a;
	}

	if (b && b->busy) {
		uint16_t cluster = 0;
		addr_mesh1d(b->pkt.to, &cluster, NULL, NULL);

		if (cluster < n->cluster)
			sel_b = b;
	}

	if (!sel_a && !sel_b)
		return;

	if (sel_a && !sel_b) {
		copy_reg(&right_out(n), sel_a);
		return;
	}

	if (!sel_a && sel_b) {
		copy_reg(&right_out(n), sel_b);
		return;
	}

	/* both available, select older */
	if (sel_a->pkt.timestamp < sel_b->pkt.timestamp)
		copy_reg(&right_out(n), sel_a);
	else
		copy_reg(&right_out(n), sel_b);
}

static void propagate(struct node1d *n, int elem, struct reg *a, struct reg *b, struct reg *c)
{

	struct reg *sel_a = NULL, *sel_b = NULL, *sel_c = NULL;
	if (a && a->busy) {
		uint16_t cluster = 0, element = 0;
		addr_mesh1d(a->pkt.to, &cluster, &element, NULL);

		if (cluster == n->cluster && element == elem)
			sel_a = a;
	}

	if (b && b->busy) {
		uint16_t cluster = 0, element = 0;
		addr_mesh1d(b->pkt.to, &cluster, &element, NULL);

		if (cluster == n->cluster && element == elem)
			sel_b = b;
	}

	if (c && c->busy) {
		uint16_t cluster = 0, element = 0;
		addr_mesh1d(c->pkt.to, &cluster, &element, NULL);

		if (cluster == n->cluster && element == elem)
			sel_c = c;
	}

	struct reg *sel_0 = NULL, *sel_1 = NULL;
	if (sel_a && sel_b)
		sel_0 = sel_a->pkt.timestamp < sel_b->pkt.timestamp ? sel_a : sel_b;
	else
		sel_0 = sel_a ? sel_a : sel_b;

	if (sel_b && sel_c)
		sel_1 = sel_b->pkt.timestamp < sel_c->pkt.timestamp ? sel_b : sel_c;
	else
		sel_1 = sel_b ? sel_b : sel_c;

	struct reg *sel = NULL;
	if (sel_0 && sel_1)
		sel = sel_0->pkt.timestamp < sel_1->pkt.timestamp ? sel_0 : sel_1;
	else
		sel = sel_0 ? sel_0 : sel_1;

	if (!sel)
		return;

	copy_reg(&n->out[elem], sel);
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

	propagate_left(n, r, &right_in(n));
	propagate_right(n, r, &left_in(n));
	for (int i = 0; i < n->elems; ++i)
		propagate(n, i, r, &right_in(n), &left_in(n));

	return OK;
}

stat mesh_node1d_connect(struct component *c, struct component *e, uint16_t elem)
{
	struct node1d *n = (struct node1d *)c;
	if (elem >= n->elems)
		return ENOSUCH;

	if (n->ports[elem])
		return EEXISTS;

	n->ports[elem] = e;
	return OK;
}

stat mesh_node1d_connect_left(struct component *c, struct component *e)
{
	struct node1d *n = (struct node1d *)c;
	if (left_port(n))
		return EEXISTS;

	left_port(n) = e;
	return OK;
}

stat mesh_node1d_connect_right(struct component *c, struct component *e)
{
	struct node1d *n = (struct node1d *)c;
	if (right_port(n))
		return EEXISTS;

	right_port(n) = e;
	return OK;
}

struct component *create_mesh_node1d(uint16_t cluster, uint16_t elems)
{
	struct node1d *n = (struct node1d *)calloc(1, sizeof(struct node1d));
	if (!n)
		return NULL;

	n->in = (struct reg *)calloc(elems + 2, sizeof(struct reg));
	if (!n->in) {
		free(n);
		return NULL;
	}

	n->out = (struct reg *)calloc(elems + 2, sizeof(struct reg));
	if (!n->out) {
		free(n->in);
		free(n);
		return NULL;
	}

	n->ports = (struct component **)calloc(elems + 2, sizeof(struct component *));
	if (!n->ports) {
		free(n->out);
		free(n->in);
		free(n);
		return NULL;
	}

	n->component.destroy = (destroy_callback)node1d_destroy;
	n->component.receive = (receive_callback)node1d_receive;
	n->component.clock   = (clock_callback)node1d_clock;
	n->cluster = cluster;
	n->elems = elems;
	return (struct component *)n;
}
