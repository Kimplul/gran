SIMPLE_RV64	:= tests/simple_riscv64
SIMPLE_RV64_SIM	:= $(SIMPLE_RV64)/sim.c

TESTS += $(SIMPLE_RV64)/sim

$(SIMPLE_RV64)/sim: $(SIMPLE_RV64_SIM) libgran.a
	mkdir -p build/$(SIMPLE_RV64)
	$(COMPILE_TEST) $(SIMPLE_RV64_SIM) libgran.a -o build/$@
	./scripts/gen-report -d build $@
