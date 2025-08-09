#ifndef GRAN_MESH_NODE3D_H
#define GRAN_MESH_NODE3D_H

#include <gran/component.h>
#include <stdint.h>

struct component *create_mesh_node3d(uint8_t x, uint8_t y, uint8_t z, uint8_t elem);

stat mesh_node3d_connect(struct component *c, struct component *e, uint8_t elem);
stat mesh_node3d_connect_north(struct component *c, struct component *e);
stat mesh_node3d_connect_south(struct component *c, struct component *e);
stat mesh_node3d_connect_east(struct component *c, struct component *e);
stat mesh_node3d_connect_west(struct component *c, struct component *e);
stat mesh_node3d_connect_down(struct component *c, struct component *e);
stat mesh_node3d_connect_up(struct component *c, struct component *e);

static inline uint64_t mesh3d_addr(uint8_t x, uint8_t y, uint8_t z, uint8_t elem,
                                   uint32_t off)
{
	return off
	       | ((uint64_t)elem << 32)
	       | ((uint64_t)x << 40)
	       | ((uint64_t)y << 48)
	       | ((uint64_t)z << 56)
	;
}

static inline void addr_mesh3d(uint64_t addr, uint8_t *x, uint8_t *y, uint8_t *z, uint8_t *elem, uint32_t *off)
{
	if (off) *off = addr & 0xffffffff;
	if (elem) *elem = (addr >> 32) & 0xff;
	if (x) *x = (addr >> 40) & 0xff;
	if (y) *y = (addr >> 48) & 0xff;
	if (z) *z = (addr >> 56) & 0xff;
}

#endif /* GRAN_GRID_NODE3D_H */
