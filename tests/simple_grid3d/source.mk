GRID3D_TEST_OBJ		!= ./scripts/gen-deps --sources "tests/simple_grid3d/sim.c"
TEST_PROGS		+= build/tests/simple_grid3d/sim

build/tests/simple_grid3d/test.inc: tests/simple_grid3d/test.c
	riscv64-unknown-elf-gcc -O2 -Wall -Wextra -ffreestanding -nostdlib \
		-march=rv64i -mabi=lp64 \
		-o build/tests/simple_grid3d/test \
		tests/simple_grid3d/test.c
	riscv64-unknown-elf-objcopy -Obinary \
		build/tests/simple_grid3d/test \
		build/tests/simple_grid3d/test.bin
	xxd -i build/tests/simple_grid3d/test.bin \
		> build/tests/simple_grid3d/test.inc

build/tests/simple_grid3d/sim.o: build/tests/simple_grid3d/test.inc

build/tests/simple_grid3d/sim: $(GRID3D_TEST_OBJ) $(OBJS)
	$(COMPILE) $(GRID3D_TEST_OBJ) $(OBJS) -o $@
