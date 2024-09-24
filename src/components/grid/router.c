#include <stdbool.h>

#include <gran/grid/node.h>
#include <gran/grid/router.h>
#include <gran/vec.h>

struct router_region {
	uint32_t addr;
	uint32_t size;
	struct component *component;
};

struct node_router {
	struct component component;
	struct component *ascend;
	struct vec regions;
	uint8_t u, v, x, y;

	struct component *send;
	struct packet pkt;
	bool busy;
};

static struct router_region *find_region(struct node_router *router, uint32_t addr)
{
	for (size_t i = 0; i < vec_len(&router->regions); ++i) {
		struct router_region *region = vec_at(&router->regions, i);

		if (addr >= region->addr && addr < region->addr + region->size)
			return region;
	}

	return NULL;
}

static stat router_clock(struct node_router *router)
{
	if (!router->busy)
		return OK;

	stat r = SEND(router, router->send, router->pkt);
	if (r == EBUSY)
		return OK;

	router->busy = false;
	return OK;
}

static stat router_receive(struct node_router *router, struct component *from, struct packet pkt)
{
	if (router->busy)
		return EBUSY;

	router->busy = true;

	uint64_t addr = pkt.to;
	uint8_t u;
	uint8_t v;
	uint8_t x;
	uint8_t y;
	addr_grid(addr, NULL, &x, &y, &u, &v);

	router->pkt = pkt;

	if (router->u != u || router->v != v || router->x != x || router->y != y) {
		if (!router->ascend)
			goto nosuch;

		router->send = router->ascend;
		return OK;
	}

	struct router_region *region = find_region(router, addr);
	if (!region)
		goto nosuch;

	router->send = region->component;
	return OK;

nosuch:
	router->send = from;
	router->pkt = response(pkt);
	set_flags(&router->pkt, PACKET_ERROR);
	return OK;
}

struct component *create_node_router(uint8_t u, uint8_t v, uint8_t x, uint8_t y)
{
	struct node_router *router = calloc(1, sizeof(struct node_router));
	if (!router)
		return NULL;

	router->u = u;
	router->v = v;
	router->x = x;
	router->y = y;

	router->component.receive = (receive_callback)router_receive;
	router->component.clock = (clock_callback)router_clock;
	router->regions = vec_create(sizeof(struct router_region));

	return (struct component *)router;
}

stat node_router_add(struct component *router, struct component *component, uint32_t addr, uint32_t size)
{
	struct node_router *nr = (struct node_router *)router;
	struct router_region *found = find_region(nr, addr);
	if (!found)           found = find_region(nr, addr + size);

	if (found) {
		error("%s overlaps with %s at %x",
				found->component->name,
				component->name,
				found->addr
		     );

		return EEXISTS;
	}

	struct router_region r = (struct router_region){
		.addr = addr,
		.size = size,
		.component = component
	};
	vect_append(struct node_region, nr->regions, &r);

	return OK;
}

stat node_router_ascend(struct component *router, struct component *node)
{
	struct node_router *nr = (struct node_router *)router;
	nr->ascend = node;
	return OK;
}
