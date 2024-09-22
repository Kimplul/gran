/* compile with
 *	riscv64-unknown-elf-gcc -ffreestanding -nostdlib -Wl,-Ttext=0 -march=rv64i -mabi=lp64 -O2 hello.c -o simple_hello
 *	xxd --include simple_hello
 */
void _start()
{
	/* relies on hard-coded system, good enough for this simple test */
	volatile char *uart = (char *)4096;
	*uart = 'H';
	*uart = 'e';
	*uart = 'l';
	*uart = 'l';
	*uart = 'o';
	*uart = '!';
	*uart = '\n';

	asm ("ebreak\n");
}
