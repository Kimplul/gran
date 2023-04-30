#ifndef EXSIM_COMPONENT_H
#define EXSIM_COMPONENT_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <gran/common.h>

struct component;

typedef stat (*write_callback)(struct component *, uintptr_t, size_t, void *);
typedef stat (*read_callback)(struct component *, uintptr_t, size_t, void *);
typedef stat (*swap_callback)(struct component *, uintptr_t, size_t, void *,
                              size_t, void *);
typedef stat (*irq_callback)(struct component *, int);
typedef stat (*clock_callback)(struct component *);

typedef stat (*stat_callback)(struct component *, FILE *);
typedef stat (*dts_callback)(struct component *, FILE *);

typedef void (*destroy_callback)(struct component *);

struct component {
	/* optional */
	char *name;

	write_callback write;
	read_callback read;
	swap_callback swap;

	irq_callback irq;
	clock_callback clock;

	dts_callback dts;
	stat_callback stat;

	destroy_callback destroy;
};

static inline stat write(struct component *component, uintptr_t addr,
                         size_t size, void *buf)
{
	if (!component->write) {
		error(
			"tried writing %p:%" PRIuPTR " but it doesn't support writing",
			component->name, addr);
		return ENOSUCH;
	}

	return component->write(component, addr, size, buf);
}

static inline stat read(struct component *component, uintptr_t addr,
                        size_t size, void *buf)
{
	if (!component->read) {
		error(
			"tried reading %p:%" PRIuPTR " but it doesn't support reading",
			component->name, addr);
		return ENOSUCH;
	}

	return component->read(component, addr, size, buf);
}

static inline stat swap(struct component *component, uintptr_t addr,
                        size_t wsize, void *wbuf, size_t rsize, void *rbuf)
{
	if (!component->swap) {
		error(
			"tried swapping %p:%" PRIuPTR " but it doesn't support swapping",
			component->name, addr);
		return ENOSUCH;
	}

	return component->swap(component, addr, wsize, wbuf, rsize, rbuf);
}

static inline void destroy(struct component *component)
{
	if (component->destroy)
		component->destroy(component);
	else
		free(component);
}

static inline stat write_u8(struct component *component, uintptr_t addr,
                            uint8_t c)
{
	return write(component, addr, sizeof(uint8_t), &c);
}

static inline stat read_u8(struct component *component, uintptr_t addr,
                           uint8_t *c)
{
	return read(component, addr, sizeof(uint8_t), c);
}

static inline stat swap_u8(struct component *component, uintptr_t addr,
                           uint8_t *c)
{
	return swap(component, addr, sizeof(uint8_t), c, sizeof(uint8_t), c);
}

#endif /* EXSIM_COMPONENT_H */
