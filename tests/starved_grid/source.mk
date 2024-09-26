STARVED_TEST_OBJ	!= ./scripts/gen-deps --sources "tests/starved_grid/sim.c"

TEST_PROGS	+= build/tests/starved_grid/sim

build/tests/starved_grid/sim: $(STARVED_TEST_OBJ) $(OBJS)
	$(COMPILE) $(STARVED_TEST_OBJ) $(OBJS) -o $@
