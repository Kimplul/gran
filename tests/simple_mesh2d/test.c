__attribute__((always_inline))
static inline void print_int8(volatile char *uart, unsigned x)
{
	*uart = ((x >> 4) & 0xf) + '0';
	*uart = ((x >> 0) & 0xf) + '0';
}

__attribute__((always_inline))
static inline void print_addr(volatile char *uart, unsigned x, unsigned y)
{
	*uart = '(';
	print_int8(uart, x);
	*uart = ',';
	*uart = ' ';
	print_int8(uart, y);
	*uart = ')';
	*uart = '\n';
}

__attribute__((always_inline))
static inline unsigned wrap(unsigned x, unsigned X)
{
	return x + 1 >= X ? 0 : x + 1;
}

__attribute__((always_inline))
static inline unsigned next_idx(unsigned x, unsigned y, unsigned X, unsigned Y)
{
	unsigned yi = wrap(y, Y);
	unsigned xi = yi < y ? wrap(x, X) : x;

	return (xi << 16) | yi;
}

void _start(unsigned x, unsigned y, unsigned X, unsigned Y)
{
	volatile char *uart = (char *)4096;
	volatile unsigned *control = (unsigned *)(1ULL << 56);

	if (x == 0 && y == 2) {
		goto do_work;
	} else {
		while (*control != ((x << 16) | y)) {}
	}

do_work:
	print_addr(uart, x, y);
	*control = next_idx(x, y, X, Y);

	if (x == X - 1 && y == Y - 1)
		asm ("ebreak");

	/* otherwise just loop */
	while (1) {}
}
