#define X 16
#define Y 16

void _start(unsigned short x, unsigned short y)
{
	volatile unsigned long *counter = (unsigned long *)((unsigned long)1 <<
	                                                    48);
	volatile char *uart = (char *)4096;

	/* very hacky, not recommended but good enough for testing */
	if (x == 0 && y == 2) {
		*counter = 0;
		*uart = '0';
		*uart = '\n';
	}
	else
		while (*counter != (((unsigned long)x << 16) | y))
			;

	*uart = '(';
	/* [0 - 16] as two octal numbers */
	*uart = ((x >> 3) & 0x7) + '0';
	*uart = ((x >> 0) & 0x7) + '0';

	*uart = ',';
	*uart = ' ';

	*uart = ((y >> 3) & 0x7) + '0';
	*uart = ((y >> 0) & 0x7) + '0';

	*uart = ')';
	*uart = '\n';

	if (x == 15 && y == 15)
		asm ("ebreak\n");

	if (y == 15) {
		x++;
		y = 0;
	}
	else
		y++;

	*counter = ((unsigned long)x << 16) | y;
	while (1)
		;
}
