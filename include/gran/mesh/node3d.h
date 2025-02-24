#ifndef GRAN_MESH_NODE3D_H
#define GRAN_MESH_NODE3D_H

#include <gran/component.h>
#include <stdint.h>

struct component *create_mesh_node3d(uint8_t x, uint8_t y, uint8_t z);

stat mesh_node3d_connect(struct component *node,
		struct component *n,
		struct component *s,
		struct component *e,
		struct component *w,
		struct component *u,
		struct component *d,
		struct component *l);

static inline uint64_t mesh3d_addr(uint8_t x, uint8_t y, uint8_t z, uint32_t off)
{
	return off
		| ((uint64_t)x << 32)
		| ((uint64_t)y << 40)
		| ((uint64_t)z << 48)
		;
}

static inline void addr_mesh3d(uint64_t addr, uint8_t *x, uint8_t *y, uint8_t *z, uint32_t *off)
{
	if (off) *off = addr & 0xffffffff;
	if (x) *x = (addr >> 32) & 0xff;
	if (y) *y = (addr >> 40) & 0xff;
	if (z) *z = (addr >> 48) & 0xff;
	assert(((addr >> 56) & 0xff) == 0);
}

#endif /* GRAN_GRID_NODE3D_H */
