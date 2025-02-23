#include <assert.h>

#include <gran/root.h>
#include <gran/mem/simple_mem.h>
#include <gran/bus/simple_bus.h>
#include <gran/uart/simple_uart.h>
#include <gran/grid/node3d.h>
#include <gran/cpu/riscv/simple_riscv64.h>

#include "../build/tests/simple_grid3d/test.inc"

static size_t idx_1d(int x, int y, int z, uint8_t xw, uint8_t yw, uint8_t zw)
{
	(void)xw; /* maybe unused */
	assert(0 <= x && x < xw);
	assert(0 <= y && y < yw);
	assert(0 <= z && z < zw);
	return (x * yw * zw) + (y * zw) + z;
}

static struct component *grid_at(struct component **grid,
		int x, int y, int z,
		uint8_t xw, uint8_t yw, uint8_t zw)
{
	if (x < 0)
		return NULL;

	if (y < 0)
		return NULL;

	if (z < 0)
		return NULL;

	if (x >= xw)
		return NULL;

	if (y >= yw)
		return NULL;

	if (z >= zw)
		return NULL;

	return grid[idx_1d(x, y, z, xw, yw, zw)];
}

static void connect_grid3d(struct component **grid, struct component *c,
		uint8_t x,  uint8_t y,  uint8_t z,
		uint8_t xw, uint8_t yw, uint8_t zw)
{
	grid_node3d_connect(grid_at(grid, x, y, z, xw, yw, zw),
			grid_at(grid, x  , y+1, z  , xw, yw, zw),
			grid_at(grid, x  , y-1, z  , xw, yw, zw),
			grid_at(grid, x-1, y  , z  , xw, yw, zw),
			grid_at(grid, x+1, y  , z  , xw, yw, zw),
			grid_at(grid, x  , y  , z+1, xw, yw, zw),
			grid_at(grid, x  , y  , z-1, xw, yw, zw),
			c);
}

static stat build_torus3d(struct clock_domain *clk, uint8_t x, uint8_t y, uint8_t z)
{
	struct component **grid = calloc(x * y * z, sizeof(struct component *));
	assert(grid);

	struct component **pes = calloc(x * y * z, sizeof(struct component *));
	assert(pes);

	for (size_t i = 0; i < x; ++i)
	for (size_t j = 0; j < y; ++j)
	for (size_t k = 0; k < z; ++k) {
		struct component *node = create_grid_node3d(i, j, k);
		clock_domain_add(clk, node);
		grid[idx_1d(i, j, k, x, y, z)] = node;

		if (i == 0 && j == 0 && k == 0)
			continue;

		if (i == 0 && j == 0 && k == 1)
			continue;

		struct component *imem = create_simple_mem(4096);
		init_simple_mem(imem, 0,
				build_tests_simple_grid3d_test_bin_len,
				build_tests_simple_grid3d_test_bin);

		uint64_t rcv = grid3d_addr(i, j, k, 0);
		struct component *rv64 = create_simple_riscv64(rcv, 0, imem, node);
		simple_riscv64_set_reg(rv64, 10, i); /* a0 */
		simple_riscv64_set_reg(rv64, 11, j); /* a1 */
		simple_riscv64_set_reg(rv64, 12, k); /* a2 */
		simple_riscv64_set_reg(rv64, 13, x); /* a3 */
		simple_riscv64_set_reg(rv64, 14, y); /* a4 */
		simple_riscv64_set_reg(rv64, 15, z); /* a5 */

		clock_domain_add(clk, rv64);
		clock_domain_add(clk, imem);

		pes[idx_1d(i, j, k, x, y, z)] = rv64;
	}

	struct component *uart = create_simple_uart();
	clock_domain_add(clk, uart);
	connect_grid3d(grid, uart, 0, 0, 0, x, y, z);

	struct component *dmem = create_simple_mem(4096);
	clock_domain_add(clk, dmem);
	connect_grid3d(grid, dmem, 0, 0, 1, x, y, z);

	for (int i = 0; i < x; ++i)
	for (int j = 0; j < y; ++j)
	for (int k = 0; k < z; ++k) {
		if (i == 0 && j == 0 && k == 0)
			continue;

		if (i == 0 && j == 0 && k == 1)
			continue;

		connect_grid3d(grid,
				pes[idx_1d(i, j, k, x, y, z)],
				i, j, k, x, y, z);
	}

	free(grid);
	free(pes);
	return OK;
}

int main()
{
	struct clock_domain *clk = create_clock_domain(NS(1));

	stat r = build_torus3d(clk, 4, 4, 4);
	assert(r == OK);

	struct gran_root *root = create_root();
	root_add_clock(root, clk);

	r = root_run(root);
	assert(r == OK);

	destroy_root(root);
}
