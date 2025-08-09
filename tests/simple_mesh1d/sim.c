#include <assert.h>

#include <gran/root.h>
#include <gran/mem/simple_mem.h>
#include <gran/bus/simple_bus.h>
#include <gran/uart/simple_uart.h>
#include <gran/mesh/node1d.h>
#include <gran/cpu/riscv/simple_riscv64.h>

#include "../build/tests/simple_mesh1d/test.inc"

static stat build_node1d(struct clock_domain *clk, uint16_t x, uint16_t y)
{
	struct component **mesh = calloc(x + 1, sizeof(struct component *));
	assert(mesh);

	for (int i = 1; i < x + 1; ++i) {
		struct component *node = create_mesh_node1d(i, y + 1);
		clock_domain_add(clk, node);
		mesh[i] = node;

		for (int j = 0; j < y; ++j) {
			struct component *imem = create_simple_mem(4096);
			init_simple_mem(imem, 0,
			                build_tests_simple_mesh1d_test_bin_len,
			                build_tests_simple_mesh1d_test_bin);

			uint64_t rcv = mesh1d_addr(i, j, 0);
			struct component *rv64 = create_simple_riscv64(rcv, 0,
			                                               imem, node);

			simple_riscv64_set_reg(rv64, 10, i); /* a0 */
			simple_riscv64_set_reg(rv64, 11, j); /* a1 */
			simple_riscv64_set_reg(rv64, 12, x); /* a2 */
			simple_riscv64_set_reg(rv64, 13, y); /* a3 */

			clock_domain_add(clk, rv64);
			clock_domain_add(clk, imem);

			mesh_node1d_connect(node, rv64, j);
		}

		struct component *dmem = create_simple_mem(4096);
		clock_domain_add(clk, dmem);
		mesh_node1d_connect(node, dmem, 4);
	}

	/* extra I/O node */
	struct component *node = create_mesh_node1d(0, 2);
	clock_domain_add(clk, node);
	mesh[0] = node;

	struct component *uart = create_simple_uart();
	clock_domain_add(clk, uart);
	mesh_node1d_connect(node, uart, 0);

	struct component *dmem = create_simple_mem(4096);
	clock_domain_add(clk, dmem);
	mesh_node1d_connect(node, dmem, 1);

	for (int i = 0; i < x + 1; ++i) {
		if (i - 1 >= 0)
			mesh_node1d_connect_south(mesh[i], mesh[i - 1]);

		if (i + 1 < x + 1)
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
	stat r = build_node1d(clk, 8, 8);
	assert(r == OK);

	struct gran_root *root = create_root();
	root_add_clock(root, clk);

	r = root_run(root);
	assert(r == OK);

	destroy_root(root);
}
