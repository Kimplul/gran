MESH2D_TEST_OBJ		!= ./scripts/gen-deps --sources "tests/simple_mesh2d/sim.c"
TEST_PROGS		+= build/tests/simple_mesh2d/sim

build/tests/simple_mesh2d/test.inc: tests/simple_mesh2d/test.c
	riscv64-unknown-elf-gcc -O2 -Wall -Wextra -ffreestanding -nostdlib \
		-march=rv64i -mabi=lp64 \
		-o build/tests/simple_mesh2d/test \
		tests/simple_mesh2d/test.c
	riscv64-unknown-elf-objcopy -Obinary \
		build/tests/simple_mesh2d/test \
		build/tests/simple_mesh2d/test.bin
	xxd -i build/tests/simple_mesh2d/test.bin \
		> build/tests/simple_mesh2d/test.inc

build/tests/simple_mesh2d/sim.o: build/tests/simple_mesh2d/test.inc

build/tests/simple_mesh2d/sim: $(MESH2D_TEST_OBJ) $(OBJS)
	$(COMPILE) $(MESH2D_TEST_OBJ) $(OBJS) -o $@
