SIMPLE_UART	:= tests/simple_uart
SIMPLE_UART_SIM	:= $(SIMPLE_UART)/sim.c

TEST += $(SIMPLE_UART)/sim

$(SIMPLE_UART)/sim: $(SIMPLE_UART_SIM) libgran.a
	$(COMPILE_TEST) $(SIMPLE_UART_SIM) libran.a -o build/$@
	./scripts/gen-report -d build $@
