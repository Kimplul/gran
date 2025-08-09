/* SPDX-License-Identifier: copyleft-next-0.3.1 */
/* Copyright 2023 Kim Kuparinen < kimi.h.kuparinen@gmail.com > */

#include <assert.h>

#include <gran/root.h>
#include <gran/mem/simple_mem.h>
#include <gran/bus/simple_bus.h>
#include <gran/uart/simple_uart.h>
#include <gran/cpu/riscv/simple_riscv64.h>

unsigned char simple_hello[] = {
	0xb7, 0x17, 0x00, 0x00, 0x13, 0x07, 0x80, 0x04, 0x23, 0x80, 0xe7, 0x00,
	0x13, 0x07, 0x50, 0x06, 0x23, 0x80, 0xe7, 0x00, 0x13, 0x07, 0xc0, 0x06,
	0x23, 0x80, 0xe7, 0x00, 0x23, 0x80, 0xe7, 0x00, 0x13, 0x07, 0xf0, 0x06,
	0x23, 0x80, 0xe7, 0x00, 0x13, 0x07, 0x10, 0x02, 0x23, 0x80, 0xe7, 0x00,
	0x13, 0x07, 0xa0, 0x00, 0x23, 0x80, 0xe7, 0x00, 0x73, 0x00, 0x10, 0x00,
	0x67, 0x80, 0x00, 0x00
};
unsigned int simple_hello_len = 64;

int main()
{
	const size_t size = 4096;
	struct component *imem = create_simple_mem(size);
	init_simple_mem(imem, 0, simple_hello_len, simple_hello);

	struct component *dmem = create_simple_mem(size);
	struct component *uart = create_simple_uart();
	struct component *bus = create_simple_bus();
	simple_bus_add(bus, dmem, 0, size);
	simple_bus_add(bus, uart, size, 1);

	struct component *rv64 = create_simple_riscv64(8192, 0, imem, bus);
	simple_bus_add(bus, rv64, 8192, 4096);

	struct clock_domain *clk = create_clock_domain(NS(1));
	clock_domain_add(clk, rv64);
	clock_domain_add(clk, dmem);
	clock_domain_add(clk, imem);
	clock_domain_add(clk, bus);
	clock_domain_add(clk, uart);

	struct gran_root *root = create_root();
	root_add_clock(root, clk);

	assert(root_run(root) == OK);

	destroy_root(root);
}
