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

static stat router_write(struct node_router *router, struct packet *pkt)
{
	uint64_t addr = packet_addr(pkt);
	uint8_t u = (addr >> 56) & 0xff;
	uint8_t v = (addr >> 48) & 0xff;
	uint8_t x = (addr >> 40) & 0xff;
	uint8_t y = (addr >> 32) & 0xff;

	if (router->u != u || router->v != v || router->x != x || router->y != y)
		return write(router->ascend, pkt);

	struct router_region *region = find_region(router, addr);
	if (!region)
		return write(router->ascend, pkt);

	return write(region->component, pkt);
}

static stat router_read(struct node_router *router, struct packet *pkt)
{
	uint64_t addr = packet_addr(pkt);
	uint8_t u = (addr >> 56) & 0xff;
	uint8_t v = (addr >> 48) & 0xff;
	uint8_t x = (addr >> 40) & 0xff;
	uint8_t y = (addr >> 32) & 0xff;

	if (router->u != u || router->v != v || router->x != x || router->y != y)
		return read(router->ascend, pkt);

	struct router_region *region = find_region(router, addr);
	if (!region)
		return read(router->ascend, pkt);

	return read(region->component, pkt);
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

	router->component.write = (write_callback)router_write;
	router->component.read = (read_callback)router_read;
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
