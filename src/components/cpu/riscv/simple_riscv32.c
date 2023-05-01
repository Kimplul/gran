/* SPDX-License-Identifier: copyleft-next-0.3.1 */
/* Copyright 2023 Kim Kuparinen < kimi.h.kuparinen@gmail.com > */

#include <gran/cpu/riscv/simple_riscv32.h>

struct simple_riscv32 {
	struct component component;

	struct component *imem;
	struct component *dmem;

	/* have to be careful with x0 */
	uint32_t regs[32];
	uint32_t pc;
};

static uint32_t get_reg(struct simple_riscv32 *cpu, size_t i)
{
	assert(i < 32);

	if (i == 0)
		return 0;

	return cpu->regs[i];
}

static void set_reg(struct simple_riscv32 *cpu, size_t i, uint32_t v)
{
	assert(i < 32);

	if (i == 0)
		return;

	cpu->regs[i] = v;
}

static stat simple_riscv32_clock(struct simple_riscv32 *cpu)
{
	uint32_t insn = 0;
	stat ret = read(cpu->imem, cpu->pc, sizeof(insn), &insn);
	if (ret)
		return ret;

	/* @todo instruction decode and execution, not sure if this is too early
	 * to start thinking about how to modularise stuff */
	return OK;
}

struct componen *create_simple_riscv32(uint32_t start_pc,
                                       struct component *imem,
                                       struct component *dmem)
{
	struct component *new = calloc(1, sizeof(simple_riscv32));
	if (!new)
		return NULL;

	new->component.clock = (clock_callback)simple_riscv32_clock;

	new->pc = start_pc;
	new->imem = imem;
	new->dmem = dmem;
	return new;
}
