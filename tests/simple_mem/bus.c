/* SPDX-License-Identifier: copyleft-next-0.3.1 */
/* Copyright 2023 Kim Kuparinen < kimi.h.kuparinen@gmail.com > */

#include <assert.h>

#include <gran/root.h>
#include <gran/clock_domain.h>
#include <gran/mem/simple_mem.h>
#include <gran/bus/simple_bus.h>

#include "traffic_gen.h"

int main()
{
	const size_t size = 100000;
	const uintptr_t addr = 100000;
	struct component *simple_mem = create_simple_mem(size);
	struct component *simple_bus = create_simple_bus();
	simple_bus_add(simple_bus, simple_mem, addr, size);

	struct component *traffic_gen = create_traffic_gen(simple_bus, addr, size);

	struct clock_domain *clk = create_clock_domain(NS(1));
	clock_domain_add(clk, traffic_gen);

	struct gran_root *root = create_root();
	root_add_clock(root, clk);

	assert(root_run(root) == OK);

	destroy_root(root);
}
