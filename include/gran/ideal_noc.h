#ifndef GRAN_IDEAL_NOC_H
#define GRAN_IDEAL_NOC_H

/**
 * Essentially, this is the behaviour of a NoC that everyone wants, but nobody
 * knows how to get. All-to all, single cycle, with oldest-first arbitration.
 *
 * I don't think it's been proven to be physically/mathematically
 * impossible to build such a system (with reasonable performance/area costs),
 * but at least there's no known way with existing mainstream technologies.
 * There are some interesting future research areas, like adding wireless elements
 * to the NoC, optical interconnects, superconducting routers and pulse computing,
 * but nothing definite as far as I'm aware. Still, might be fun to compare how
 * close to an idealized situation we can get, and maybe just daydream.
 */

#include <stdint.h>
#include <gran/component.h>

struct component *create_ideal_noc(uint32_t elems, size_t latency);
stat ideal_noc_connect(struct component *noc, struct component *component, uint32_t elem);

static inline void addr_ideal_noc(uint64_t addr, uint32_t *elem, uint32_t *off)
{
	if (off) *off = addr & 0xffffffff;
	if (elem) *elem = (addr >> 32) & 0xffffffff;
}

static inline uint64_t ideal_noc_addr(uint32_t elem, uint32_t off)
{
	return ((uint64_t)elem << 32) | off;
}

#endif /* GRAN_IDEAL_NOC_H */
