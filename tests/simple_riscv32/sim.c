/* SPDX-License-Identifier: copyleft-next-0.3.1 */
/* Copyright 2023 Kim Kuparinen < kimi.h.kuparinen@gmail.com > */

#include <assert.h>

#include <gran/root.h>
#include <gran/mem/simple_mem.h>
#include <gran/cpu/riscv/simple_riscv32.h>

unsigned char simple_sum[] = {
	0x13, 0x05, 0x00, 0x00, 0xb7, 0x45, 0x0f, 0x00, 0x93, 0x85, 0x05, 0x24,
	0x13, 0x06, 0x00, 0x00, 0x63, 0x08, 0xb5, 0x00, 0x33, 0x06, 0xa6, 0x00,
	0x13, 0x05, 0x15, 0x00, 0x6f, 0xf0, 0x5f, 0xff, 0x73, 0x00, 0x10, 0x00
};

unsigned int simple_sum_len = 36;

int main()
{
	const size_t size = 100000;
	struct component *imem = create_simple_mem(size);

	/* This works for simple memory, but feels kind of hacky */
	init_simple_mem(imem, 0, simple_sum_len, simple_sum);

	struct component *dmem = create_simple_mem(size);
	struct component *rv32 = create_simple_riscv32(0, imem, dmem);

	struct clock_domain *clk = create_clock_domain(NS(1));
	clock_domain_add(clk, rv32);

	struct gran_root *root = create_root();
	root_add_clock(root, clk);

	assert(root_run(root) == OK);

	destroy_root(root);
}
