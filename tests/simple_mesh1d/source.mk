SIMPLE_MESH1D		:= tests/simple_mesh1d
SIMPLE_MESH1D_SIM	:= $(SIMPLE_MESH1D)/sim.c

TESTS += $(SIMPLE_MESH1D)/sim

.PHONY: $(SIMPLE_MESH1D)/sim
$(SIMPLE_MESH1D)/sim: $(SIMPLE_MESH1D_SIM) libgran.a
	mkdir -p build/$(SIMPLE_MESH1D)
	./scripts/gen-rv64-fw -d build -o $(SIMPLE_MESH1D)/test.inc $(SIMPLE_MESH1D)/test.c
	$(COMPILE_TEST) $(SIMPLE_MESH1D_SIM) libgran.a -o build/$@
	./scripts/gen-report -d build $@
