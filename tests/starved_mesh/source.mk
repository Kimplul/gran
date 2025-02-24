STARVED_TEST_OBJ	!= ./scripts/gen-deps --sources "tests/starved_mesh/sim.c"

TEST_PROGS	+= build/tests/starved_mesh/sim

build/tests/starved_mesh/sim: $(STARVED_TEST_OBJ) $(OBJS)
	$(COMPILE) $(STARVED_TEST_OBJ) $(OBJS) -o $@
