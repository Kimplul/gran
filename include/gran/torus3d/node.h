#ifndef GRAN_TORUS3D_NODE_H
#define GRAN_TORUS3D_NODE_H

#include <gran/component.h>
#include <stdint.h>

struct component *create_torus3d_node(uint8_t x, uint8_t y, uint8_t z);

stat torus3d_node_connect(struct component *node,
		struct component *x_in,
		struct component *y_in,
		struct component *z_in,
		struct component *child,
		struct component *x_out,
		struct component *y_out,
		struct component *z_out);

static inline void addr_torus3d(uint64_t addr, uint8_t *x, uint8_t *y, uint8_t *z, uint32_t *off)
{
	if (off) *off = addr & 0xffffffff;
	if (x) *x = (addr >> 32) & 0xff;
	if (y) *y = (addr >> 40) & 0xff;
	if (z) *z = (addr >> 48) & 0xff;
	assert(((addr >> 56) & 0xff) == 0);
}

static inline uint64_t torus3d_addr(uint8_t x, uint8_t y, uint8_t z, uint32_t off)
{
	return off | ((uint64_t)x << 32)
		   | ((uint64_t)y << 40)
		   | ((uint64_t)z << 48)
		   ;
}

#endif /* GRAN_TORUS3D_NODE_H */
