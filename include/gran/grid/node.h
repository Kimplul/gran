#ifndef GRAN_GRID_NODE_H
#define GRAN_GRID_NODE_H

#include <gran/component.h>
#include <stdint.h>

struct component *create_grid_node(uint8_t u, uint8_t v, uint8_t x, uint8_t y);

stat grid_node_connect(struct component *node,
		struct component *left, struct component *right,
		struct component *up, struct component *down,
		struct component *lower, struct component *ascend);

static inline uint64_t grid_addr(uint8_t u, uint8_t v, uint8_t x, uint8_t y, uint32_t off)
{
	return off
		| ((uint64_t)x << 32)
		| ((uint64_t)y << 40)
		| ((uint64_t)u << 48)
		| ((uint64_t)v << 56)
		;
}

static inline void addr_grid(uint64_t addr, uint32_t *off, uint8_t *x, uint8_t *y, uint8_t *u, uint8_t *v)
{
	if (off) *off = addr & 0xffffffff;
	if (x) *x = (addr >> 32) & 0xff;
	if (y) *y = (addr >> 40) & 0xff;
	if (u) *u = (addr >> 48) & 0xff;
	if (v) *v = (addr >> 56) & 0xff;
}

#endif /* GRAN_GRID_NODE_H */
