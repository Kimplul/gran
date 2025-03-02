#include <assert.h>

#include <gran/root.h>
#include <gran/mem/ideal_alloc.h>

#include "testbench.h"

int main()
{
	struct component *traffic_gen = create_testbench();
	struct component *ideal_alloc = create_ideal_alloc(1);

	ideal_alloc_connect(ideal_alloc, traffic_gen);
	testbench_connect(traffic_gen, ideal_alloc);

	struct clock_domain *clk = create_clock_domain(NS(1));
	clock_domain_add(clk, traffic_gen);
	clock_domain_add(clk, ideal_alloc);

	struct gran_root *root = create_root();
	root_add_clock(root, clk);

	assert(root_run(root) == OK);

	destroy_root(root);
}
