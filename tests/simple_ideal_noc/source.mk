IDEAL_NOC_TEST_OBJ	!= ./scripts/gen-deps --sources "tests/simple_ideal_noc/sim.c"
TEST_PROGS		+= build/tests/simple_ideal_noc/sim

build/tests/simple_ideal_noc/test.inc: tests/simple_ideal_noc/test.c
	riscv64-unknown-elf-gcc -O2 -Wall -Wextra -ffreestanding -nostdlib \
		-march=rv64i -mabi=lp64 \
		-o build/tests/simple_ideal_noc/test \
		tests/simple_ideal_noc/test.c
	riscv64-unknown-elf-objcopy -Obinary \
		build/tests/simple_ideal_noc/test \
		build/tests/simple_ideal_noc/test.bin
	xxd -i build/tests/simple_ideal_noc/test.bin \
		> build/tests/simple_ideal_noc/test.inc

build/tests/simple_ideal_noc/sim.o: build/tests/simple_ideal_noc/test.inc

build/tests/simple_ideal_noc/sim: $(IDEAL_NOC_TEST_OBJ) $(OBJS)
	$(COMPILE) $(IDEAL_NOC_TEST_OBJ) $(OBJS) -o $@
