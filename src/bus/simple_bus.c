/* SPDX-License-Identifier: copyleft-next-0.3.1 */
/* Copyright 2023 Kim Kuparinen < kimi.h.kuparinen@gmail.com > */
#include <stdbool.h>

#include <gran/bus/simple_bus.h>
#include <gran/vec.h>

struct bus_region {
	uint64_t addr;
	uint64_t size;
	struct component *component;
};

struct simple_bus {
	struct component component;
	struct vec regions;

	struct component *send;
	struct packet pkt;
	bool busy;
};

static struct bus_region *find_bus_region(struct simple_bus *bus, uint64_t addr)
{
	foreach_vec(i, bus->regions) {
		struct bus_region *r = vec_at(&bus->regions, i);
		if (addr >= r->addr && addr < (r->addr + r->size))
			return r;
	}

	return NULL;
}

static stat simple_bus_clock(struct simple_bus *bus)
{
	if (bus->busy) {
		stat r = SEND(bus, bus->send, bus->pkt);
		if (r == EBUSY)
			return OK;

		bus->busy = false;
		return r;
	}

	return OK;
}

static stat simple_bus_receive(struct simple_bus *bus, struct component *from, struct packet pkt)
{
	if (bus->busy)
		return EBUSY;

	bus->busy = true;

	struct bus_region *region = find_bus_region(bus, pkt.to);
	if (!region) {
		warn("illegal address on bus %s at %" PRIuPTR,
		     bus->component.name, pkt.to);

		bus->send = from;
		bus->pkt = response(pkt);
		set_flags(&bus->pkt, PACKET_ERROR);
		return OK;
	}

	bus->send = region->component;
	bus->pkt = pkt;
	return OK;
}

static void simple_bus_destroy(struct simple_bus *bus)
{
	vec_destroy(&bus->regions);
	free(bus);
}

struct component *create_simple_bus()
{
	struct simple_bus *bus = calloc(1, sizeof(struct simple_bus));
	if (!bus)
		return NULL;

	bus->component.receive = (receive_callback)simple_bus_receive;
	bus->component.clock = (clock_callback)simple_bus_clock;

	bus->component.destroy = (destroy_callback)simple_bus_destroy;
	bus->regions = vec_create(sizeof(struct bus_region));

	return (struct component *)bus;
}

stat simple_bus_add(struct component *bus, struct component *component,
                    uint64_t addr, uint64_t size)
{
	struct simple_bus *b = (struct simple_bus *)bus;
	struct bus_region *found = find_bus_region(b, addr);
	if (!found)        found = find_bus_region(b, addr + size);

	if (found) {
		error("%s overlaps with %s at %" PRIuPTR,
		      found->component->name,
		      component->name,
		      found->addr
		      );

		return EEXISTS;
	}

	struct bus_region new = (struct bus_region){
		.component = component,
		.addr = addr,
		.size = size
	};

	vect_append(struct bus_region, b->regions, &new);
	return OK;
}
