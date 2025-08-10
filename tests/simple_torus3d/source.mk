SIMPLE_TORUS3D		:= tests/simple_torus3d
SIMPLE_TORUS3D_SIM	:= $(SIMPLE_TORUS3D)/sim.c

TESTS += $(SIMPLE_TORUS3D)/sim

.PHONY: $(SIMPLE_TORUS3D)/sim
$(SIMPLE_TORUS3D)/sim: $(SIMPLE_TORUS3D_SIM) libgran.a
	mkdir -p build/$(SIMPLE_TORUS3D)
	./scripts/gen-rv64-fw -d build -o $(SIMPLE_TORUS3D)/test.inc $(SIMPLE_TORUS3D)/test.c
	$(COMPILE_TEST) $(SIMPLE_TORUS3D_SIM) libgran.a -o build/$@
	./scripts/gen-report -d build $@
