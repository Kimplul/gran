#ifndef MESH_NODE1D_H
#define MESH_NODE1D_H

#include <stdint.h>
#include <gran/component.h>

struct component *create_mesh_node1d(uint16_t cluster, uint16_t elems);
stat mesh_node1d_connect(struct component *node1d, struct component *component,
                         uint16_t elem);

stat mesh_node1d_connect_north(struct component *c, struct component *e);
stat mesh_node1d_connect_south(struct component *c, struct component *e);

static inline void addr_mesh1d(uint64_t addr, uint16_t *cluster, uint16_t *elem,
                               uint32_t *off)
{
	if (off) *off = addr & 0xffffffff;
	if (elem) *elem = (addr >> 32) & 0xffff;
	if (cluster) *cluster = (addr >> 48) & 0xffff;
}

static inline uint64_t mesh1d_addr(uint16_t cluster, uint16_t elem,
                                   uint32_t off)
{
	return ((uint64_t)cluster << 48) | ((uint64_t)elem << 32) | off;
}

#endif /* MESH_NODE1D_H */
