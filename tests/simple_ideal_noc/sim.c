#include <assert.h>

#include <gran/root.h>
#include <gran/mem/simple_mem.h>
#include <gran/bus/simple_bus.h>
#include <gran/uart/simple_uart.h>
#include <gran/ideal_noc.h>
#include <gran/cpu/riscv/simple_riscv64.h>

#include "../build/tests/simple_ideal_noc/test.inc"

static stat build_ideal_noc(struct clock_domain *clk, uint32_t x)
{
	struct component **pes = calloc(x, sizeof(struct component *));
	assert(pes);

	/* very much ideal noc */
	struct component *noc = create_ideal_noc(x, 0);
	clock_domain_add(clk, noc);

	for (uint32_t i = 0; i < x; ++i) {
		if (i == 0 || i == 1)
			continue;

		struct component *imem = create_simple_mem(4096);
		init_simple_mem(imem, 0,
				build_tests_simple_ideal_noc_test_bin_len,
				build_tests_simple_ideal_noc_test_bin);

		uint64_t rcv = ideal_noc_addr(i, 0);
		struct component *rv64 = create_simple_riscv64(rcv, 0, imem, noc);
		simple_riscv64_set_reg(rv64, 10, i); /* a0 */
		simple_riscv64_set_reg(rv64, 11, x); /* a1 */

		clock_domain_add(clk, rv64);
		clock_domain_add(clk, imem);

		pes[i] = rv64;
	}

	struct component *uart = create_simple_uart();
	clock_domain_add(clk, uart);
	ideal_noc_connect(noc, uart, 0);

	struct component *dmem = create_simple_mem(4096);
	clock_domain_add(clk, dmem);
	ideal_noc_connect(noc, dmem, 1);

	for (size_t i = 0; i < x; ++i) {
		if (i == 0 || i == 1)
			continue;

		ideal_noc_connect(noc, pes[i], i);
	}

	free(pes);
	return OK;
}

int main()
{
	struct clock_domain *clk = create_clock_domain(NS(1));

	stat r = build_ideal_noc(clk, 64);
	assert(r == OK);

	struct gran_root *root = create_root();
	root_add_clock(root, clk);

	r = root_run(root);
	assert(r == OK);

	destroy_root(root);
}
