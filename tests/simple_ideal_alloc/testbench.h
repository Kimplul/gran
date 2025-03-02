#ifndef GRAN_IDEAL_ALLOC_TESTBENCH_H
#define GRAN_IDEAL_ALLOC_TESTBENCH_H

#include <gran/component.h>

struct component *create_testbench();
stat testbench_connect(struct component *tb, struct component *dut);

#endif /* GRAN_IDEAL_ALLOC_TESTBENCH_H */
