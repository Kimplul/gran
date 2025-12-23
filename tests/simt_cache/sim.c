#include <assert.h>

#include <gran/root.h>
#include <gran/mem/simple_mem.h>
#include <gran/bus/simple_bus.h>
#include <gran/uart/simple_uart.h>
#include <gran/mesh/node1d.h>
#include <gran/cpu/riscv/simple_riscv64.h>
#include <gran/cache/simt_cache.h>

#include "../build/tests/simt_cache/test.inc"

static stat build_simt(struct clock_domain *clk, uint16_t clusters, uint16_t lanes)
{
	struct component **mesh = calloc(clusters + 1, sizeof(struct component *));
	assert(mesh);

	for (int i = 1; i < clusters + 1; ++i) {
		struct component *node = create_mesh_node1d(i, 3);
		clock_domain_add(clk, node);
		mesh[i] = node;

		struct component *mem = create_simple_mem(4096);
		init_simple_mem(mem, 0,
				build_tests_simt_cache_test_inc_bin_len,
				build_tests_simt_cache_test_inc_bin);

		clock_domain_add(clk, mem);

		uint64_t icache_rcv = mesh1d_addr(i, 0, 0);
		struct component *icache = create_simt_cache(icache_rcv, lanes, node);
		clock_domain_add(clk, icache);

		uint64_t dcache_rcv = mesh1d_addr(i, 1, 0);
		struct component *dcache = create_simt_cache(dcache_rcv, lanes, node);
		clock_domain_add(clk, dcache);

		for (int j = 0; j < lanes; ++j) {
			struct component *rv64 = create_simple_riscv64(
					(uint64_t)j << 32,
					mesh1d_addr(i, 2, 0),
					icache,
					dcache
			);

			simple_riscv64_set_reg(rv64, 10, i); /* a0 */
			simple_riscv64_set_reg(rv64, 11, j); /* a1 */
			simple_riscv64_set_reg(rv64, 12, clusters); /* a2 */
			simple_riscv64_set_reg(rv64, 13, lanes); /* a3 */
			clock_domain_add(clk, rv64);

			simt_cache_connect_lane(icache, j, rv64);
			simt_cache_connect_lane(dcache, j, rv64);
		}

		mesh_node1d_connect(node, icache, 0);
		mesh_node1d_connect(node, dcache, 1);
		mesh_node1d_connect(node, mem, 2);
	}

	/* extra I/O node */
	struct component *node = create_mesh_node1d(0, 2);
	clock_domain_add(clk, node);
	mesh[0] = node;

	struct component *uart = create_simple_uart();
	clock_domain_add(clk, uart);
	mesh_node1d_connect(node, uart, 0);

	struct component *mem = create_simple_mem(4096);
	clock_domain_add(clk, mem);
	mesh_node1d_connect(node, mem, 1);

	for (int i = 0; i < clusters + 1; ++i) {
		if (i - 1 >= 0)
			mesh_node1d_connect_south(mesh[i], mesh[i - 1]);

		if (i + 1 < clusters + 1)
			mesh_node1d_connect_north(mesh[i], mesh[i + 1]);
	}

	free(mesh);
	return OK;
}

int main()
{
	struct clock_domain *clk = create_clock_domain(NS(1));

	/* one cluster with four cores + mem for each core (should
	 * instructions be fetched from cluster local mem?) */
	stat r = build_simt(clk, 8, 8);
	assert(r == OK);

	struct gran_root *root = create_root();
	root_add_clock(root, clk);

	r = root_run(root);
	assert(r == OK);

	destroy_root(root);
}
