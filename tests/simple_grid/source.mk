GRID_TEST_OBJ	!= ./scripts/gen-deps --sources "tests/simple_grid/sim.c"

TEST_PROGS	+= build/tests/simple_grid/sim

build/tests/simple_grid/sim: $(GRID_TEST_OBJ) $(OBJS)
	$(COMPILE) $(GRID_TEST_OBJ) $(OBJS) -o $@
