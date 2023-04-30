#ifndef GRAN_SIMPLE_RISCV32
#define GRAN_SIMPLE_RISCV32

#include <gran/component.h>

struct component *create_simple_riscv32(uint32_t start_pc, struct component *imem, struct component *dmem);

#endif /* GRAN_SIMPLE_RISCV */
