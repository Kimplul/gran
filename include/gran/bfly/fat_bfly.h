#ifndef GRAN_FAT_BFLY_H
#define GRAN_FAT_BFLY_H

#include <gran/component.h>

struct component *create_fat_bfly(uint32_t elems);
stat fat_bfly_connect(struct component *bfly, struct component *component, uint32_t elem);

static inline void addr_fat_bfly(uint64_t addr, uint32_t *elem, uint32_t *off)
{
	if (off) *off = addr & 0xffffffff;
	if (elem) *elem = (addr >> 32) & 0xffffffff;
}

static inline uint64_t fat_bfly_addr(uint32_t elem, uint32_t off)
{
	return ((uint64_t)elem << 32) | off;
}

#endif /* GRAN_FAT_BFLY_H */
