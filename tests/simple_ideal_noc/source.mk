SIMPLE_IDEAL_NOC	:= tests/simple_ideal_noc
SIMPLE_IDEAL_NOC_SIM	:= $(SIMPLE_IDEAL_NOC)/sim.c

TESTS += $(SIMPLE_IDEAL_NOC)/sim

.PHONY: $(SIMPLE_IDEAL_NOC)/sim
$(SIMPLE_IDEAL_NOC)/sim: $(SIMPLE_IDEAL_NOC_SIM) libgran.a
	mkdir -p build/$(SIMPLE_IDEAL_NOC)
	./scripts/gen-rv64-fw -d build -o $(SIMPLE_IDEAL_NOC)/test.inc $(SIMPLE_IDEAL_NOC)/test.c
	$(COMPILE_TEST) $(SIMPLE_IDEAL_NOC_SIM) libgran.a -o build/$@
	./scripts/gen-report -d build $@
