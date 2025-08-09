/* SPDX-License-Identifier: copyleft-next-0.3.1 */
/* Copyright 2023 Kim Kuparinen < kimi.h.kuparinen@gmail.com > */

/**
 * @file clock_domain.c
 *
 * Clock domain implementation stuff.
 */

#include <stdlib.h>
#include <threads.h>
#include <assert.h>

#include <gran/clock_domain.h>

#define VEC_NAME components
#define VEC_TYPE struct component *
#include <conts/vec.h>

struct clock_domain {
	struct components components;
	struct clock_time time;
	tick period;

	stat ret;
};

void advance_clock(struct clock_domain *clk)
{
	tick fs = clk->time.fs + clk->period;
	while (fs >= SEC(1)) {
		clk->time.s++;
		fs -= SEC(1);
	}
	clk->time.fs = fs;
}

struct clock_domain *create_clock_domain(tick period)
{
	struct clock_domain *clk = calloc(1, sizeof(struct clock_domain));
	if (!clk)
		return NULL;

	clk->period = period;
	clk->components = components_create(0);

	return clk;
}

void destroy_clock_domain(struct clock_domain *clk)
{
	foreach(components, c, &clk->components) {
		destroy(*c);
	}

	components_destroy(&clk->components);
	free(clk);
}

stat clock_domain_add(struct clock_domain *clk, struct component *component)
{
	components_append(&clk->components, component);
	return OK;
}

static void clocked_component_tick(struct clock_domain *clk,
                                   struct component *component)
{
	stat r = component->clock(component);
	if (r)
		clk->ret = r;
}

stat clock_domain_tick(struct clock_domain *clk)
{
	foreach(components, c, &clk->components) {
		clocked_component_tick(clk, *c);
	}

	advance_clock(clk);
	return clk->ret;
}

bool eq_time(struct clock_time a, struct clock_time b)
{
	return a.s == b.s && a.fs == b.fs;
}

bool lt_time(struct clock_time a, struct clock_time b)
{
	if (a.s == b.s)
		return a.fs < b.fs;

	return a.s < b.s;
}

bool le_time(struct clock_time a, struct clock_time b)
{
	if (a.s == b.s)
		return a.fs == b.fs || a.fs < b.fs;

	return a.s < b.s;
}

struct clock_time max_time(struct clock_time a, struct clock_time b)
{
	if (lt_time(a, b))
		return b;

	return a;
}

struct clock_time domain_time(struct clock_domain *clk)
{
	assert(clk);
	return clk->time;
}
