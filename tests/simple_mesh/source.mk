MESH_TEST_OBJ		!= ./scripts/gen-deps --sources "tests/simple_mesh/sim.c"
TEST_PROGS		+= build/tests/simple_mesh/sim

build/tests/simple_mesh/test.inc: tests/simple_mesh/test.c
	riscv64-unknown-elf-gcc -O2 -Wall -Wextra -ffreestanding -nostdlib \
		-march=rv64i -mabi=lp64 \
		-o build/tests/simple_mesh/test \
		tests/simple_mesh/test.c
	riscv64-unknown-elf-objcopy -Obinary \
		build/tests/simple_mesh/test \
		build/tests/simple_mesh/test.bin
	xxd -i build/tests/simple_mesh/test.bin \
		> build/tests/simple_mesh/test.inc

build/tests/simple_mesh/sim.o: build/tests/simple_mesh/test.inc

build/tests/simple_mesh/sim: $(MESH_TEST_OBJ) $(OBJS)
	$(COMPILE) $(MESH_TEST_OBJ) $(OBJS) -o $@
