SIMT_RISCV64		:= tests/simt_riscv64
SIMT_RISCV64_SIM	:= $(SIMT_RISCV64)/sim.c

TESTS += $(SIMT_RISCV64)/sim

.PHONY: $(SIMT_RISCV64)/sim
$(SIMT_RISCV64)/sim: $(SIMT_RISCV64_SIM) libgran.a
	mkdir -p build/$(SIMT_RISCV64)
	./scripts/gen-rv64-fw -d build -o $(SIMT_RISCV64)/test.inc $(SIMT_RISCV64)/test.c
	$(COMPILE_TEST) $(SIMT_RISCV64_SIM) libgran.a -o build/$@
	./scripts/gen-report -d build $@
