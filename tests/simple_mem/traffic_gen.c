#include <stdlib.h>
#include <assert.h>

#include <gran/component.h>

#include "traffic_gen.h"

struct traffic_gen {
	struct component component;
	struct component *stress;
	uintptr_t addr;
	uintptr_t end;

	size_t counter;
};

static stat traffic_gen_clock(struct traffic_gen *tg)
{
	if (tg->addr == tg->end)
		return DONE;

	uint8_t c = 0;
	switch (tg->counter) {
	case 0:
		assert(write_u8(tg->stress, tg->addr, 13) == OK);
		tg->counter = 1;
		break;

	case 1:
		c = 0;
		assert(read_u8(tg->stress, tg->addr, &c) == OK);
		assert(c == 13);
		tg->counter = 0;
		tg->addr++;
		break;
	}

	return OK;
}

void traffic_gen_destroy(struct traffic_gen *tg)
{
	destroy(tg->stress);
	free(tg);
}

struct component *create_traffic_gen(struct component *stress, uintptr_t start, size_t size)
{
	struct traffic_gen *new = calloc(1, sizeof(struct traffic_gen));
	if (!new)
		return NULL;

	new->stress = stress;
	new->addr = start;
	new->end = start + size;
	new->counter = 0;

	new->component.clock = (clock_callback)traffic_gen_clock;
	new->component.destroy = (destroy_callback)traffic_gen_destroy;
	return (struct component *)new;
}
