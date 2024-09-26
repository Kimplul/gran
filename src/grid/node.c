/* very simple grid node with 32bit private region, does not currently signal
 * being busy or anything. I think I might have to refine the message passing
 * interface I have, but this is good enough.
 *
 * Each node should have a router beneath it, just to simplify my life. A router
 * is basically a bus with a fallback ascension path.
 */
#include <stdbool.h>

#include <gran/grid/node.h>

struct port {
	struct component *send;
	struct packet pkt;
	bool busy;
};

struct grid_node {
	struct component component;
	uint16_t x, y;

	struct port left, right, up, down, lower;

	unsigned priority;
};

static stat port_clock(struct grid_node *grid, struct port *to, struct port *from)
{
	assert(from->busy);
	stat r = SEND(grid, to->send, from->pkt);
	if (r == EBUSY) {
		grid->priority++;
		return OK;
	}

	from->busy = false;
	return OK;
}

static struct port *select_input(struct grid_node *grid)
{
	struct port *ports[5] = {
		&grid->left,
		&grid->right,
		&grid->up,
		&grid->down,
		&grid->lower,
	};
	for (size_t i = 0; i < 5; ++i) {
		size_t idx = (i + grid->priority) % 5;
		struct port *port = ports[idx];
		if (!port)
			continue;

		if (!port->busy)
			continue;

		return port;
	}

	return NULL;
}

static stat clock_once(struct grid_node *grid)
{
	struct port *input = select_input(grid);
	if (!input)
		return OK;

	uint16_t x;
	uint16_t y;
	uint64_t addr = input->pkt.to;
	addr_grid(addr, NULL, &x, &y);

	if (grid->x == x && grid->y == y) {
		if (!grid->lower.send)
			goto nosuch;

		return port_clock(grid, &grid->lower, input);
	}

	if (y < grid->y) {
		if (!grid->down.send)
			goto nosuch;

		return port_clock(grid, &grid->down, input);
	}

	if (y > grid->y) {
		if (!grid->up.send)
			goto nosuch;

		return port_clock(grid, &grid->up, input);
	}

	if (x < grid->x) {
		if (!grid->left.send)
			goto nosuch;

		return port_clock(grid, &grid->left, input);
	}

	if (x > grid->x) {
		if (!grid->right.send)
			goto nosuch;

		return port_clock(grid, &grid->right, input);
	}

nosuch:
	abort();
	return OK;
}

static stat grid_clock(struct grid_node *grid)
{
	clock_once(grid);
	return OK;
}


static stat port_receive(struct port *port, struct packet pkt)
{
	if (port->busy)
		return EBUSY;

	port->pkt = pkt;
	port->busy = true;
	return OK;
}

static stat grid_receive(struct grid_node *grid, struct component *from, struct packet pkt)
{
	if (from == grid->lower.send)
		return port_receive(&grid->lower, pkt);

	if (from == grid->left.send)
		return port_receive(&grid->left, pkt);

	if (from == grid->right.send)
		return port_receive(&grid->right, pkt);

	if (from == grid->down.send)
		return port_receive(&grid->down, pkt);

	if (from == grid->up.send)
		return port_receive(&grid->up, pkt);

	abort();
	return OK;
}

struct component *create_grid_node(uint16_t x, uint16_t y)
{
	struct grid_node *node = calloc(1, sizeof(struct grid_node));
	if (!node)
		return NULL;

	node->component.receive = (receive_callback)grid_receive;
	node->component.clock = (clock_callback)grid_clock;
	node->x = x;
	node->y = y;
	return (struct component *)node;
}

stat grid_node_connect(struct component *node,
		struct component *left, struct component *right,
		struct component *up, struct component *down,
		struct component *lower)
{
	struct grid_node *n = (struct grid_node *)node;
	n->left.send = left;
	n->right.send = right;
	n->up.send = up;
	n->down.send = down;
	n->lower.send = lower;
	return OK;
}
