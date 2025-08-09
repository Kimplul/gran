MESH1D_TEST_OBJ	!= ./scripts/gen-deps --sources "tests/simple_mesh1d/sim.c"
TEST_PROGS	+= build/tests/simple_mesh1d/sim

build/tests/simple_mesh1d/test.inc: tests/simple_mesh1d/test.c
	riscv64-unknown-elf-gcc -O2 -Wall -Wextra -ffreestanding -nostdlib \
		-march=rv64i -mabi=lp64 \
		-fno-delete-null-pointer-checks \
		-o build/tests/simple_mesh1d/test \
		tests/simple_mesh1d/test.c
	riscv64-unknown-elf-objcopy -Obinary \
		build/tests/simple_mesh1d/test \
		build/tests/simple_mesh1d/test.bin
	xxd -i build/tests/simple_mesh1d/test.bin \
		> build/tests/simple_mesh1d/test.inc

build/tests/simple_mesh1d/sim.o: build/tests/simple_mesh1d/test.inc

build/tests/simple_mesh1d/sim: $(MESH1D_TEST_OBJ) $(OBJS)
	$(COMPILE) $(MESH1D_TEST_OBJ) $(OBJS) -o $@
