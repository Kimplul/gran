#ifndef GRAN_GRID_NODE_H
#define GRAN_GRID_NODE_H

#include <gran/component.h>
#include <stdint.h>

struct component *create_grid_node(uint16_t x, uint16_t y);

stat grid_node_connect(struct component *node,
		struct component *left, struct component *right,
		struct component *up, struct component *down,
		struct component *lower);

static inline uint64_t grid_addr(uint16_t x, uint16_t y, uint32_t off)
{
	return off
		| ((uint64_t)x << 32)
		| ((uint64_t)y << 48)
		;
}

static inline void addr_grid(uint64_t addr, uint32_t *off, uint16_t *x, uint16_t *y)
{
	if (off) *off = addr & 0xffffffff;
	if (x) *x = (addr >> 32) & 0xffff;
	if (y) *y = (addr >> 48) & 0xffff;
}

#endif /* GRAN_GRID_NODE_H */
