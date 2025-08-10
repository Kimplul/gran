STARVED_MESH		:= tests/starved_mesh
STARVED_MESH_SIM	:= $(STARVED_MESH)/sim.c

TESTS += $(STARVED_MESH)/sim

$(STARVED_MESH)/sim: $(STARVED_MESH_SIM) libgran.a
	mkdir -p build/$(STARVED_MESH)
	$(COMPILE_TEST) $(STARVED_MESH_SIM) libgran.a -o build/$@
	./scripts/gen-report -d build $@
