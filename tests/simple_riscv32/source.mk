RV32_TEST_OBJ	!= ./scripts/gen-deps --sources "tests/simple_riscv32/sim.c"

TEST_PROGS	+= build/tests/simple_riscv32/sim

build/tests/simple_riscv32/sim: $(RV32_TEST_OBJ) $(TEST_OBJS) $(OBJS)
	$(COMPILE) $(RV32_TEST_OBJ) $(TEST_OBJS) $(OBJS) -o $@
