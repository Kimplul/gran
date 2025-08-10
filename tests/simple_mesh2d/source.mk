SIMPLE_MESH2D		:= tests/simple_mesh2d
SIMPLE_MESH2D_SIM	:= $(SIMPLE_MESH2D)/sim.c

TESTS += $(SIMPLE_MESH2D)/sim

.PHONY: $(SIMPLE_MESH2D)/sim
$(SIMPLE_MESH2D)/sim: $(SIMPLE_MESH2D_SIM) libgran.a
	mkdir -p build/$(SIMPLE_MESH2D)
	./scripts/gen-rv64-fw -d build -o $(SIMPLE_MESH2D)/test.inc $(SIMPLE_MESH2D)/test.c
	$(COMPILE_TEST) $(SIMPLE_MESH2D_SIM) libgran.a -o build/$@
	./scripts/gen-report -d build $@
