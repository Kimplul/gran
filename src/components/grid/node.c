/* very simple grid node with 32bit private region, does not currently signal
 * being busy or anything. I think I might have to refine the message passing
 * interface I have, but this is good enough.
 *
 * Each node should have a router beneath it, just to simplify my life. A router
 * is basically a bus with a fallback ascension path.
 */
#include <gran/grid/node.h>

struct grid_node {
	struct component component;
	uint8_t u, v, x, y;
	struct component *left, *right, *up, *down, *ascend, *lower;
};

typedef read_callback callback;

static stat grid_route(struct grid_node *grid, struct packet *pkt, callback op)
{
	uint64_t addr = packet_addr(pkt);
	uint8_t u = (addr >> 56) & 0xff;
	uint8_t v = (addr >> 48) & 0xff;
	uint8_t x = (addr >> 40) & 0xff;
	uint8_t y = (addr >> 32) & 0xff;

	if (grid->u == u && grid->v == v && grid->x == x && grid->y == y)
		return op(grid->lower, pkt);

	if (grid->u != u || grid->v != v) {
		if (!grid->ascend)
			return EBUS;

		return op(grid->ascend, pkt);
	}

	if (y < grid->y) {
		if (!grid->down)
			return EBUS;

		return op(grid->down, pkt);
	}

	if (y > grid->y) {
		if (!grid->up)
			return EBUS;

		return op(grid->up, pkt);
	}

	if (x < grid->x) {
		if (!grid->left)
			return EBUS;

		return op(grid->left, pkt);
	}

	if (x > grid->x) {
		if (!grid->right)
			return EBUS;

		return op(grid->right, pkt);
	}

	return EBUS;
}

static stat grid_write(struct grid_node *node, struct packet *pkt)
{
	return grid_route(node, pkt, write);
}

static stat grid_read(struct grid_node *node, struct packet *pkt)
{
	return grid_route(node, pkt, read);
}

struct component *create_grid_node(uint8_t u, uint8_t v, uint8_t x, uint8_t y)
{
	struct grid_node *node = calloc(1, sizeof(struct grid_node));
	if (!node)
		return NULL;

	node->u = u;
	node->v = v;
	node->x = x;
	node->y = y;
	node->component.write = (write_callback)grid_write;
	node->component.read = (read_callback)grid_read;

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
