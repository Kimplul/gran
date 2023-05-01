/* SPDX-License-Identifier: copyleft-next-0.3.1 */
/* Copyright 2023 Kim Kuparinen < kimi.h.kuparinen@gmail.com > */

#include <stdlib.h>
#include <threads.h>
#include <assert.h>

#include <gran/clock_domain.h>

#define MAX_COMPONENTS 512

struct clock_domain {
	size_t num_components;
	struct component *components[MAX_COMPONENTS];
	struct clock_time time;
	tick period;

	stat ret;

	mtx_t mtx;
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
	mtx_init(&clk->mtx, mtx_plain);

	return clk;
}

void destroy_clock_domain(struct clock_domain *clk)
{
	for (size_t i = 0; i < clk->num_components; ++i)
		destroy(clk->components[i]);

	free(clk);
}

stat clock_domain_add(struct clock_domain *clk, struct component *component)
{
	clk->components[clk->num_components++] = component;
	return OK;
}

static void update_ret(struct clock_domain *clk, stat ret)
{
	if (ret) {
		mtx_lock(&clk->mtx);
		clk->ret = ret;
		mtx_unlock(&clk->mtx);
	}
}

static void clocked_component_tick(struct clock_domain *clk,
                                   struct component *component)
{
	stat ret = component->clock(component);
	update_ret(clk, ret);
}

stat clock_domain_tick(struct clock_domain *clk)
{
	/* openmp on gcc apparently doesn't really handle if conditionals
	 * particularly efficiently, so it's faster to do it manually */
	if (clk->num_components == 1) {
		clocked_component_tick(clk, clk->components[0]);
		advance_clock(clk);
		return clk->ret;
	}

#pragma omp parallel for
	for (size_t i = 0; i < clk->num_components; ++i)
		clocked_component_tick(clk, clk->components[i]);

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
