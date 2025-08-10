SIMPLE_BAT_FLY		:= tests/simple_fat_bfly
SIMPLE_BAT_FLY_SIM	:= $(SIMPLE_BAT_FLY)/sim.c

TESTS += $(SIMPLE_BAT_FLY)/sim

.PHONY: $(SIMPLE_BAT_FLY)/sim
$(SIMPLE_BAT_FLY)/sim: $(SIMPLE_BAT_FLY_SIM) libgran.a
	mkdir -p build/$(SIMPLE_BAT_FLY)
	./scripts/gen-rv64-fw -d build -o $(SIMPLE_BAT_FLY)/test.inc $(SIMPLE_BAT_FLY)/test.c
	$(COMPILE_TEST) $(SIMPLE_BAT_FLY_SIM) libgran.a -o build/$@
	./scripts/gen-report -d build $@
