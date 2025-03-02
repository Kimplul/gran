ALLOC_TB_OBJ		!= ./scripts/gen-deps --sources "tests/simple_ideal_alloc/testbench.c"
IDEAL_ALLOC_TEST_OBJ	!= ./scripts/gen-deps --sources "tests/simple_ideal_alloc/sim.c"

TEST_PROGS		+= build/tests/simple_ideal_alloc/sim

build/tests/simple_ideal_alloc/sim: $(ALLOC_TB_OBJ) $(IDEAL_ALLOC_TEST_OBJ) $(OBJS)
	$(COMPILE) $(ALLOC_TB_OBJ) $(IDEAL_ALLOC_TEST_OBJ) $(TEST_OBJS) $(OBJS) -o $@
