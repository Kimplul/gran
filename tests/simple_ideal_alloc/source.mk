SIMPLE_IDEAL_ALLOC	:= tests/simple_ideal_alloc
SIMPLE_IDEAL_ALLOC_SIM	:= $(SIMPLE_IDEAL_ALLOC)/testbench.c $(SIMPLE_IDEAL_ALLOC)/sim.c

TESTS += $(SIMPLE_IDEAL_ALLOC)/sim

$(SIMPLE_IDEAL_ALLOC)/sim: $(SIMPLE_IDEAL_ALLOC_SIM) libgran.a
	mkdir -p build/$(SIMPLE_IDEAL_ALLOC)
	$(COMPILE_TEST) $(SIMPLE_IDEAL_ALLOC_SIM) libgran.a -o build/$@
	./scripts/gen-report -d build $@
