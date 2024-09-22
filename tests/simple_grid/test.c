#define X 64
#define Y 64

void _start(unsigned char x, unsigned char y)
{
	volatile unsigned long *counter = (unsigned long *)4096;
	volatile char *uart = (char *)8192;

	/* very hacky, not recommended but good enough for testing */
	if (x == 0 && y == 0)
		*counter = 0;
	else
		while (*counter != ((x << 8) | y))
			;

	*uart = '(';
	/* [0 - 63] as two octal numbers */
	*uart = ((x >> 3) & 0x7) + '0';
	*uart = ((x >> 0) & 0x7) + '0';

	*uart = ',';
	*uart = ' ';

	*uart = ((y >> 3) & 0x7) + '0';
	*uart = ((y >> 0) & 0x7) + '0';

	*uart = ')';
	*uart = '\n';

	if (y == 63) {
		x++;
		y = 0;
	}
	else
		y++;

	*counter = (x << 8) | y;

	while (*counter != (64 << 8))
		;

	asm ("ebreak\n");
}
