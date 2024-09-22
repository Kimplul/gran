#ifndef GRAN_GRID_NODE_H
#define GRAN_GRID_NODE_H

#include <gran/component.h>
#include <stdint.h>

struct component *create_grid_node(uint8_t u, uint8_t v, uint8_t x, uint8_t y);

stat grid_node_connect(struct component *node,
		struct component *left, struct component *right,
		struct component *up, struct component *down,
		struct component *lower, struct component *ascend);

#endif /* GRAN_GRID_NODE_H */
