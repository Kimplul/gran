UART_TEST_OBJ	!= ./scripts/gen-deps --sources "tests/simple_uart/sim.c"

TEST_PROGS	+= build/tests/simple_uart/sim

build/tests/simple_uart/sim: $(UART_TEST_OBJ) $(OBJS)
	$(COMPILE) $(UART_TEST_OBJ) $(TEST_OBJS) $(OBJS) -o $@
