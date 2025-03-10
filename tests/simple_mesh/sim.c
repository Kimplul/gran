#include <assert.h>

#include <gran/root.h>
#include <gran/mem/simple_mem.h>
#include <gran/bus/simple_bus.h>
#include <gran/uart/simple_uart.h>
#include <gran/mesh/node.h>
#include <gran/cpu/riscv/simple_riscv64.h>

#include "../build/tests/simple_mesh/test.inc"

static size_t idx_1d(int x, int y, uint8_t xw, uint8_t yw)
{
	(void)xw; /* maybe unused */
	assert(0 <= x && x < xw);
	assert(0 <= y && y < yw);
	return (x * yw) + y;
}

static struct component *mesh_at(struct component **mesh,
		int x, int y,
		uint8_t xw, uint8_t yw)
{
	if (x < 0)
		return NULL;

	if (y < 0)
		return NULL;

	if (x >= xw)
		return NULL;

	if (y >= yw)
		return NULL;

	return mesh[idx_1d(x, y, xw, yw)];
}

static void connect_mesh(struct component **mesh, struct component *c,
		uint16_t x,  uint16_t y,
		uint16_t xw, uint16_t yw)
{
	mesh_node_connect(mesh_at(mesh, x, y, xw, yw),
			mesh_at(mesh, x  , y+1, xw, yw),
			mesh_at(mesh, x  , y-1, xw, yw),
			mesh_at(mesh, x+1, y  , xw, yw),
			mesh_at(mesh, x-1, y  , xw, yw),
			c);
}

static stat build_mesh(struct clock_domain *clk, uint16_t x, uint16_t y)
{
	struct component **mesh = calloc(x * y, sizeof(struct component *));
	assert(mesh);

	struct component **pes = calloc(x * y, sizeof(struct component *));
	assert(pes);

	for (size_t i = 0; i < x; ++i)
	for (size_t j = 0; j < y; ++j) {
		struct component *node = create_mesh_node(i, j);
		clock_domain_add(clk, node);
		mesh[idx_1d(i, j, x, y)] = node;

		if (i == 0 && j == 0)
			continue;

		if (i == 0 && j == 1)
			continue;

		struct component *imem = create_simple_mem(4096);
		init_simple_mem(imem, 0,
				build_tests_simple_mesh_test_bin_len,
				build_tests_simple_mesh_test_bin);

		uint64_t rcv = mesh_addr(i, j, 0);
		struct component *rv64 = create_simple_riscv64(rcv, 0, imem, node);
		simple_riscv64_set_reg(rv64, 10, i); /* a0 */
		simple_riscv64_set_reg(rv64, 11, j); /* a1 */
		simple_riscv64_set_reg(rv64, 12, x); /* a3 */
		simple_riscv64_set_reg(rv64, 13, y); /* a4 */

		clock_domain_add(clk, rv64);
		clock_domain_add(clk, imem);

		pes[idx_1d(i, j, x, y)] = rv64;
	}

	struct component *uart = create_simple_uart();
	clock_domain_add(clk, uart);
	connect_mesh(mesh, uart, 0, 0, x, y);

	struct component *dmem = create_simple_mem(4096);
	clock_domain_add(clk, dmem);
	connect_mesh(mesh, dmem, 0, 1, x, y);

	for (int i = 0; i < x; ++i)
	for (int j = 0; j < y; ++j) {
		if (i == 0 && j == 0)
			continue;

		if (i == 0 && j == 1)
			continue;

		connect_mesh(mesh, pes[idx_1d(i, j, x, y)], i, j, x, y);
	}

	free(mesh);
	free(pes);
	return OK;
}

int main()
{
	struct clock_domain *clk = create_clock_domain(NS(1));

	stat r = build_mesh(clk, 8, 8);
	assert(r == OK);

	struct gran_root *root = create_root();
	root_add_clock(root, clk);

	r = root_run(root);
	assert(r == OK);

	destroy_root(root);
}
