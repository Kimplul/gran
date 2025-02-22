TORUS3D_TEST_OBJ	!= ./scripts/gen-deps --sources "tests/simple_torus3d/sim.c"
TEST_PROGS		+= build/tests/simple_torus3d/sim

build/tests/simple_torus3d/test.inc: tests/simple_torus3d/test.c
	riscv64-unknown-elf-gcc -O2 -Wall -Wextra -ffreestanding -nostdlib \
		-march=rv64i -mabi=lp64 \
		-o build/tests/simple_torus3d/test \
		tests/simple_torus3d/test.c
	riscv64-unknown-elf-objcopy -Obinary \
		build/tests/simple_torus3d/test \
		build/tests/simple_torus3d/test.bin
	xxd -i build/tests/simple_torus3d/test.bin \
		> build/tests/simple_torus3d/test.inc

build/tests/simple_torus3d/sim.o: build/tests/simple_torus3d/test.inc

build/tests/simple_torus3d/sim: $(TORUS3D_TEST_OBJ) $(OBJS)
	$(COMPILE) $(TORUS3D_TEST_OBJ) $(OBJS) -o $@
