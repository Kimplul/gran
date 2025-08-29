/* SPDX-License-Identifier: copyleft-next-0.3.1 */
/* Copyright 2023 Kim Kuparinen < kimi.h.kuparinen@gmail.com > */

#include <assert.h>

#include <gran/root.h>
#include <gran/clock_domain.h>
#include <gran/mem/simple_mem.h>

#include "traffic_gen.h"

int main()
{
	const size_t size = 100000;
	struct component *simple_mem1 = create_simple_mem(size);
	struct component *simple_mem2 = create_simple_mem(size);

	struct component *traffic_gen1 =
		create_traffic_gen(simple_mem1, 0, size);
	struct component *traffic_gen2 =
		create_traffic_gen(simple_mem2, 0, size);

	struct clock_domain *clk = create_clock_domain(NS(1));
	clock_domain_add(clk, traffic_gen1);
	clock_domain_add(clk, traffic_gen2);
	clock_domain_add(clk, simple_mem1);
	clock_domain_add(clk, simple_mem2);

	struct gran_root *root = create_root();
	root_add_clock(root, clk);

	assert(root_run(root) == OK);

	destroy_root(root);
}
