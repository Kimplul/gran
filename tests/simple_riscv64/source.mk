RV64_TEST_OBJ	!= ./scripts/gen-deps --sources "tests/simple_riscv64/sim.c"

TEST_PROGS	+= build/tests/simple_riscv64/sim

build/tests/simple_riscv64/sim: $(RV64_TEST_OBJ) $(TEST_OBJS) $(OBJS)
	$(COMPILE) $(RV64_TEST_OBJ) $(TEST_OBJS) $(OBJS) -o $@
