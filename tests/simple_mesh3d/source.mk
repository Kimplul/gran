SIMPLE_MESH3D		:= tests/simple_mesh3d
SIMPLE_MESH3D_SIM	:= $(SIMPLE_MESH3D)/sim.c

TESTS += $(SIMPLE_MESH3D)/sim

.PHONY: $(SIMPLE_MESH3D)/sim
$(SIMPLE_MESH3D)/sim: $(SIMPLE_MESH3D_SIM) libgran.a
	mkdir -p build/$(SIMPLE_MESH3D)
	./scripts/gen-rv64-fw -d build -o $(SIMPLE_MESH3D)/test.inc $(SIMPLE_MESH3D)/test.c
	$(COMPILE_TEST) $(SIMPLE_MESH3D_SIM) libgran.a -o build/$@
	./scripts/gen-report -d build $@
