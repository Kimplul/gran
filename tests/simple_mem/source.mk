TRAFFIC_OBJ	!= ./scripts/gen-deps --sources "tests/simple_mem/traffic_gen.c"
NO_BUS_TEST_OBJ	!= ./scripts/gen-deps --sources "tests/simple_mem/no_bus.c"
BUS_TEST_OBJ	!= ./scripts/gen-deps --sources "tests/simple_mem/bus.c"
MANY_TEST_OBJ	!= ./scripts/gen-deps --sources "tests/simple_mem/many.c"

TEST_PROGS	+= build/tests/simple_mem/no_bus \
		   build/tests/simple_mem/bus \
		   build/tests/simple_mem/many

build/tests/simple_mem/no_bus: $(NO_BUS_TEST_OBJ) $(TRAFFIC_OBJ) $(OBJS)
	$(COMPILE) $(NO_BUS_TEST_OBJ) $(TRAFFIC_OBJ) $(OBJS) -o $@

build/tests/simple_mem/bus: $(BUS_TEST_OBJ) $(TRAFFIC_OBJ) $(OBJS)
	$(COMPILE) $(BUS_TEST_OBJ) $(TRAFFIC_OBJ) $(OBJS) -o $@

build/tests/simple_mem/many: $(MANY_TEST_OBJ) $(TRAFFIC_OBJ) $(OBJS)
	$(COMPILE) $(MANY_TEST_OBJ) $(TRAFFIC_OBJ) $(OBJS) -o $@
