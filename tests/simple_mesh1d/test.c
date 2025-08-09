#include <stdint.h>

__attribute__((always_inline))
static inline uint64_t mesh1d_addr(uint16_t cluster, uint16_t elem,
                                         uint32_t off)
{
	return ((uint64_t)cluster << 48) | ((uint64_t)elem << 32) | off;
}

__attribute__((always_inline))
static inline void print_int8(volatile char *uart, unsigned x)
{
	char lo_nibble = (x >> 0) & 0xf;
	char hi_nibble = (x >> 4) & 0xf;

	*uart = hi_nibble < 10 ? hi_nibble + '0' : (hi_nibble - 10) + 'a';
	*uart = lo_nibble < 10 ? lo_nibble + '0' : (lo_nibble - 10) + 'a';
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
static inline uint64_t wrap(unsigned x, unsigned X)
{
	return x + 1 >= X ? 0 : x + 1;
}

__attribute__((always_inline))
static inline uint64_t next_idx(unsigned x, unsigned y, unsigned X, unsigned Y)
{
	unsigned yi = wrap(y, Y);
	unsigned xi = yi < y ? wrap(x, X) : x;

	return mesh1d_addr(xi, yi, 0);
}

void _start(unsigned x, unsigned y, unsigned X, unsigned Y)
{
	volatile char *uart = (char *)mesh1d_addr(0, 0, 0);
	volatile uint64_t *control = (uint64_t *)mesh1d_addr(0, 1, 0);

	if (x == 1 && y == 0) {
		goto do_work;
	} else {
		while (*control != mesh1d_addr(x, y, 0)) {}
	}

do_work:
	print_addr(uart, x, y);
	*control = next_idx(x, y, X, Y);

	if (x == X - 1 && y == Y - 1)
		asm ("ebreak");

	/* otherwise just loop */
	while (1) {}
}
