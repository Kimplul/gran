#ifndef GRAN_GRID_NODE_H
#define GRAN_GRID_NODE_H

#include <gran/component.h>
#include <stdint.h>

struct component *create_mesh_node(uint16_t x, uint16_t y);

stat mesh_node_connect(struct component *node,
		struct component *n,
		struct component *s,
		struct component *e,
		struct component *w,
		struct component *l);

static inline uint64_t mesh_addr(uint16_t x, uint16_t y, uint32_t off)
{
	return off
		| ((uint64_t)x << 32)
		| ((uint64_t)y << 48)
		;
}

static inline void addr_mesh(uint64_t addr, uint16_t *x, uint16_t *y, uint32_t *off)
{
	if (off) *off = addr & 0xffffffff;
	if (x) *x = (addr >> 32) & 0xffff;
	if (y) *y = (addr >> 48) & 0xffff;
}

#endif /* GRAN_GRID_NODE_H */
