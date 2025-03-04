FAT_BFLY_TEST_OBJ	!= ./scripts/gen-deps --sources "tests/simple_fat_bfly/sim.c"
TEST_PROGS		+= build/tests/simple_fat_bfly/sim

build/tests/simple_fat_bfly/test.inc: tests/simple_fat_bfly/test.c
	riscv64-unknown-elf-gcc -O2 -Wall -Wextra -ffreestanding -nostdlib \
		-march=rv64i -mabi=lp64 \
		-o build/tests/simple_fat_bfly/test \
		tests/simple_fat_bfly/test.c
	riscv64-unknown-elf-objcopy -Obinary \
		build/tests/simple_fat_bfly/test \
		build/tests/simple_fat_bfly/test.bin
	xxd -i build/tests/simple_fat_bfly/test.bin \
		> build/tests/simple_fat_bfly/test.inc

build/tests/simple_fat_bfly/sim.o: build/tests/simple_fat_bfly/test.inc

build/tests/simple_fat_bfly/sim: $(FAT_BFLY_TEST_OBJ) $(OBJS)
	$(COMPILE) $(FAT_BFLY_TEST_OBJ) $(OBJS) -o $@
