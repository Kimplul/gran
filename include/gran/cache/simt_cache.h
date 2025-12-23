#ifndef GRAN_SIMT_CACHE_H
#define GRAN_SIMT_CACHE_H

#include <gran/component.h>

struct component *create_simt_cache(uint64_t rcv, size_t num_lanes, struct component *mem);
void simt_cache_connect_lane(struct component *c, size_t i, struct component *l);

#endif /* GRAN_SIMT_CACHE_H */
