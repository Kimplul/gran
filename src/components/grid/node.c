/* very simple grid node with 32bit private region, does not currently signal
 * being busy or anything. I think I might have to refine the message passing
 * interface I have, but this is good enough.
 *
 * Each node should have a router beneath it, just to simplify my life. A router
 * is basically a bus with a fallback ascension path.
 */
#include <stdbool.h>

#include <gran/grid/node.h>

struct grid_node {
	struct component component;
	uint8_t u, v, x, y;
	struct component *left, *right, *up, *down, *ascend, *lower;

	struct component *send;
	struct packet pkt;
	bool busy;
};

static stat grid_clock(struct grid_node *grid)
{
	if (!grid->busy)
		return OK;

	stat r = SEND(grid, grid->send, grid->pkt);
	if (r == EBUSY)
		return OK;

	grid->busy = false;
	return r;
}

static stat grid_receive(struct grid_node *grid, struct component *from, struct packet pkt)
{
	if (grid->busy)
		return EBUSY;

	uint8_t u;
	uint8_t v;
	uint8_t x;
	uint8_t y;
	uint64_t addr = pkt.to;
	addr_grid(addr, NULL, &x, &y, &u, &v);

	grid->busy = true;
	grid->pkt = pkt;

	if (grid->u == u && grid->v == v && grid->x == x && grid->y == y) {
		if (!grid->lower)
			goto nosuch;

		grid->send = grid->lower;
		return OK;
	}

	if (grid->u != u || grid->v != v) {
		if (!grid->ascend)
			goto nosuch;

		grid->send = grid->ascend;
		return OK;
	}

	if (y < grid->y) {
		if (!grid->down)
			goto nosuch;

		grid->send = grid->down;
		return OK;
	}

	if (y > grid->y) {
		if (!grid->up)
			goto nosuch;

		grid->send = grid->up;
		return OK;
	}

	if (x < grid->x) {
		if (!grid->left)
			goto nosuch;

		grid->send = grid->left;
		return OK;
	}

	if (x > grid->x) {
		if (!grid->right)
			return EBUS;

		grid->send = grid->right;
		return OK;
	}

nosuch:
	grid->send = from;
	grid->pkt = response(pkt);
	set_flags(&grid->pkt, PACKET_ERROR);
	return OK;
}

struct component *create_grid_node(uint8_t u, uint8_t v, uint8_t x, uint8_t y)
{
	struct grid_node *node = calloc(1, sizeof(struct grid_node));
	if (!node)
		return NULL;

	node->component.receive = (receive_callback)grid_receive;
	node->component.clock = (clock_callback)grid_clock;
	node->u = u;
	node->v = v;
	node->x = x;
	node->y = y;
	return (struct component *)node;
}

stat grid_node_connect(struct component *node,
		struct component *left, struct component *right,
		struct component *up, struct component *down,
		struct component *lower, struct component *ascend)
{
	struct grid_node *n = (struct grid_node *)node;
	n->left = left;
	n->right = right;
	n->up = up;
	n->down = down;
	n->lower = lower;
	n->ascend = ascend;
	return OK;
}
