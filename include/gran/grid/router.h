#ifndef GRAN_NODE_ROUTER_H
#define GRAN_NODE_ROUTER_H

#include <gran/component.h>

struct component *create_node_router(uint8_t u, uint8_t v, uint8_t x, uint8_t y);
stat node_router_add(struct component *router, struct component *component, uint32_t addr, uint32_t size);
stat node_router_ascend(struct component *router, struct component *node);

#endif /* GRAN_NODE_ROUTER_H */
