__attribute__((always_inline))
static inline void print_int8(volatile char *uart, unsigned x)
{
	*uart = ((x >> 4) & 0xf) + '0';
	*uart = ((x >> 0) & 0xf) + '0';
}

__attribute__((always_inline))
static inline void print_addr(volatile char *uart, unsigned x, unsigned y, unsigned z)
{
	*uart = '(';
	print_int8(uart, x);
	*uart = ',';
	*uart = ' ';
	print_int8(uart, y);
	*uart = ',';
	*uart = ' ';
	print_int8(uart, z);
	*uart = ')';
	*uart = '\n';
}

__attribute__((always_inline))
static inline unsigned wrap(unsigned x, unsigned X)
{
	return x + 1 >= X ? 0 : x + 1;
}

__attribute__((always_inline))
static inline unsigned next_idx(unsigned x, unsigned y, unsigned z, unsigned X, unsigned Y, unsigned Z)
{
	unsigned zi = wrap(z, Z);
	unsigned yi = zi < z ? wrap(y, Y) : y;
	unsigned xi = yi < y ? wrap(x, X) : x;

	return (xi << 16) | (yi << 8) | zi;
}

void _start(unsigned x, unsigned y, unsigned z, unsigned X, unsigned Y, unsigned Z)
{
	volatile char *uart = (char *)4096;
	/* x = 1ULL << 32, y = 1ULL << 40, z = 1ULL << 48 I guess */
	volatile unsigned *control = (unsigned *)(1ULL << 48);


	if (x == 0 && y == 0 && z == 2) {
		goto do_work;
	} else {
		while (*control != ((x << 16) | (y << 8) | z)) {}
	}

do_work:
	print_addr(uart, x, y, z);
	*control = next_idx(x, y, z, X, Y, Z);

	if (x == X - 1 && y == Y - 1 && z == Z - 1)
		asm("ebreak");

	/* otherwise just loop */
	while (1) {}
}
