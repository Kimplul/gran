#include <threads.h>
#include <inttypes.h>
#include <stdlib.h>

#include <gran/common.h>
#include <gran/component.h>

#include <gran/bus/simple_bus.h>

struct mem_region {
	uintptr_t addr;
	size_t size;
	struct component *component;
	struct mem_region *next;
};

struct simple_bus {
	struct component component;
	mtx_t lock;

	struct mem_region *mem_regions;
};

static struct mem_region *find_mem_region(struct simple_bus *bus,
                                          uintptr_t addr)
{
	if (!bus->mem_regions)
		return NULL;

	struct mem_region *cur = bus->mem_regions;
	while (cur) {
		/* address is within memory region */
		if (addr >= cur->addr && addr < cur->addr + cur->size)
			return cur;

		cur = cur->next;
	}

	return NULL;
}

static stat add_mem_region(struct simple_bus *bus, struct mem_region *new)
{
	if (!bus->mem_regions) {
		bus->mem_regions = new;
		return OK;
	}

	struct mem_region *found = NULL;
	if ((found = find_mem_region(bus, new->addr))) {
		error("%s overlaps with %s at %" PRIuPTR,
		      new->component->name,
		      found->component->name,
		      new->addr
		      );
		return EEXISTS;
	}

	new->next = bus->mem_regions;
	bus->mem_regions = new;
	return OK;
}

static stat simple_bus_write(struct simple_bus *bus, uintptr_t addr,
                             size_t size, char *buf)
{
	/* only one device can drive the bus at one time */
	if (mtx_trylock(&bus->lock) != thrd_success)
		return EBUSY;

	struct mem_region *mem_region = find_mem_region(bus, addr);
	if (!mem_region) {
		warn("nothing to write on bus %s at %" PRIuPTR,
		     bus->component.name, addr);
		return EBUS;
	}

	stat ret = write(mem_region->component, addr, size, buf);

	mtx_unlock(&bus->lock);
	return ret;
}

static stat simple_bus_read(struct simple_bus *bus, uintptr_t addr, size_t size,
                            char *buf)
{
	/* only one device can drive the bus at one time */
	if (mtx_trylock(&bus->lock) != thrd_success)
		return EBUSY;

	struct mem_region *mem_region = find_mem_region(bus, addr);
	if (!mem_region) {
		warn("nothing to read on bus %s at %" PRIuPTR,
		     bus->component.name, addr);
		return EBUS;
	}

	stat ret = read(mem_region->component, addr, size, buf);

	mtx_unlock(&bus->lock);
	return ret;
}

static stat simple_bus_swap(struct simple_bus *bus, uintptr_t addr,
                            size_t wsize, char *wbuf, size_t rsize, char *rbuf)
{
	if (mtx_trylock(&bus->lock) != thrd_success)
		return EBUSY;

	struct mem_region *mem_region = find_mem_region(bus, addr);
	if (!mem_region) {
		warn("nothing to swap on bus %s at %" PRIuPTR,
		     bus->component.name, addr);
		return EBUS;
	}

	stat ret = swap(mem_region->component, addr, wsize, wbuf, rsize, rbuf);

	mtx_unlock(&bus->lock);
	return ret;
}

struct component *create_simple_bus()
{
	struct simple_bus *bus = calloc(1, sizeof(struct simple_bus));
	if (!bus)
		return NULL;

	bus->component.write = (write_callback)simple_bus_write;
	bus->component.read = (read_callback)simple_bus_read;
	bus->component.swap = (swap_callback)simple_bus_swap;

	mtx_init(&bus->lock, mtx_plain);
	return (struct component *)bus;
}

stat simple_bus_add(struct component *bus, struct component *component,
                    uintptr_t addr, size_t size)
{
	struct mem_region *new = calloc(1, sizeof(struct mem_region));
	if (!new)
		return ENOMEM;

	new->addr = addr;
	new->size = size;
	new->component = component;

	if (add_mem_region((struct simple_bus *)bus, new)) {
		free(new);
		return EEXISTS;
	}

	return OK;
}
