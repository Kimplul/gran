#include <assert.h>

#include <gran/root.h>
#include <gran/mem/simple_mem.h>
#include <gran/bus/simple_bus.h>
#include <gran/uart/simple_uart.h>
#include <gran/mesh/node1d.h>
#include <gran/cpu/riscv/simt_riscv64.h>

#include "../build/tests/simt_riscv64/test.inc"

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
				build_tests_simt_riscv64_test_inc_bin_len,
				build_tests_simt_riscv64_test_inc_bin);

		uint64_t data_rcv = mesh1d_addr(i, 0, 0);
		uint64_t inst_rcv = mesh1d_addr(i, 1, 0);
		struct simt_riscv64_conf conf = {
			.data_rcv  = data_rcv,
			.inst_rcv  = inst_rcv,
			.num_lanes = lanes,
			/* each cluster should request instructions from its own
			 * instruction memory bank */
			.start_pc  = mesh1d_addr(i, 2, 0)
		};

		struct component *rv64 = create_simt_riscv64(conf, node, node);
		for (int j = 0; j < lanes; ++j) {
			simt_riscv64_set_reg(rv64, j, 10, i); /* a0 */
			simt_riscv64_set_reg(rv64, j, 11, j); /* a1 */
			simt_riscv64_set_reg(rv64, j, 12, clusters); /* a2 */
			simt_riscv64_set_reg(rv64, j, 13, lanes); /* a3 */
		}

		clock_domain_add(clk, rv64);
		clock_domain_add(clk, mem);

		mesh_node1d_connect(node, simt_riscv64_data_intf(rv64), 0);
		mesh_node1d_connect(node, simt_riscv64_inst_intf(rv64), 1);

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
	init_simple_mem(mem, 0,
			build_tests_simt_riscv64_test_inc_bin_len,
			build_tests_simt_riscv64_test_inc_bin);

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
