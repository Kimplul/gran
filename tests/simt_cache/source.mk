SIMT_CACHE	:= tests/simt_cache
SIMT_CACHE_SIM	:= $(SIMT_CACHE)/sim.c

TESTS += $(SIMT_CACHE)/sim

.PHONY: $(SIMT_CACHE)/sim
$(SIMT_CACHE)/sim: $(SIMT_CACHE_SIM) libgran.a
	mkdir -p build/$(SIMT_CACHE)
	./scripts/gen-rv64-fw -d build -o $(SIMT_CACHE)/test.inc $(SIMT_CACHE)/test.c
	$(COMPILE_TEST) $(SIMT_CACHE_SIM) libgran.a -o build/$@
	./scripts/gen-report -d build $@
