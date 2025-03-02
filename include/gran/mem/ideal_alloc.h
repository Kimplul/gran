#ifndef GRAN_IDEAL_ALLOC_H
#define GRAN_IDEAL_ALLOC_H

#include <stdint.h>
#include <gran/if/alloc.h>
#include <gran/component.h>

struct component *create_ideal_alloc(uint64_t spaces);
void ideal_alloc_connect(struct component *alloc, struct component *send);

#endif /* GRAN_IDEAL_ALLOC_H */
