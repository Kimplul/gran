__attribute__((always_inline))
static inline char hex_char(unsigned x)
{
	if (x <= 9)
		return x + '0';

	return (x - 10) + 'a';
}

__attribute__((always_inline))
static inline void print_int32(volatile char *uart, unsigned x)
{
	*uart = hex_char((x >> 28) & 0xf);
	*uart = hex_char((x >> 24) & 0xf);
	*uart = hex_char((x >> 20) & 0xf);
	*uart = hex_char((x >> 16) & 0xf);
	*uart = hex_char((x >> 12) & 0xf);
	*uart = hex_char((x >>  8) & 0xf);
	*uart = hex_char((x >>  4) & 0xf);
	*uart = hex_char((x >>  0) & 0xf);
}

__attribute__((always_inline))
static inline void print_addr(volatile char *uart, unsigned x)
{
	*uart = '(';
	print_int32(uart, x);
	*uart = ')';
	*uart = '\n';
}

void _start(unsigned x, unsigned X)
{
	volatile char *uart = (char *)4096;
	volatile unsigned *control = (unsigned *)(1ULL << 32);

	if (x == 2) {
		goto do_work;
	} else {
		while (*control != x) {}
	}

do_work:
	print_addr(uart, x);
	*control = x + 1;

	if (x == X - 1)
		asm("ebreak");

	/* otherwise just loop */
	while (1) {}
}
