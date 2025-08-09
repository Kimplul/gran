#ifndef GRAN_MESH_NODE2D_H
#define GRAN_MESH_NODE2D_H

#include <gran/component.h>
#include <stdint.h>

struct component *create_mesh_node2d(uint8_t x, uint8_t y, uint16_t elems);

stat mesh_node2d_connect(struct component *c, struct component *e,
                         uint16_t elem);
stat mesh_node2d_connect_north(struct component *c, struct component *e);
stat mesh_node2d_connect_south(struct component *c, struct component *e);
stat mesh_node2d_connect_east(struct component *c, struct component *e);
stat mesh_node2d_connect_west(struct component *c, struct component *e);

static inline uint64_t mesh2d_addr(uint8_t x, uint8_t y, uint16_t elem,
                                   uint32_t off)
{
	return off
	       | ((uint64_t)elem << 32)
	       | ((uint64_t)x << 48)
	       | ((uint64_t)y << 56)
	;
}

static inline void addr_mesh2d(uint64_t addr, uint8_t *x, uint8_t *y,
                               uint16_t *elem, uint32_t *off)
{
	if (off) *off = addr & 0xffffffff;
	if (elem) *elem = (addr >> 32) & 0xffff;
	if (x) *x = (addr >> 48) & 0xff;
	if (y) *y = (addr >> 56) & 0xff;
}

#endif /* GRAN_MESH_NODE_H */
