#include <assert.h>

#include <gran/root.h>
#include <gran/clock_domain.h>
#include <gran/mem/simple_mem.h>

#include "traffic_gen.h"

int main()
{
	const size_t size = 100000;
	struct component *simple_mem = create_simple_mem(size);
	struct component *traffic_gen = create_traffic_gen(simple_mem, 0, size);

	struct clock_domain *clk = create_clock_domain(NS(1));
	clock_domain_add(clk, traffic_gen);

	struct gran_root *root = create_root();
	root_add_clock(root, clk);

	assert(root_run(root) == OK);

	destroy_root(root);
}
