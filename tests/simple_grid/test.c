#define X 16
#define Y 16

void _start(unsigned short x, unsigned short y)
{
	volatile unsigned long *control = (unsigned long *)(((unsigned long)x << 32) | ((unsigned long)y << 48) + 128);
	volatile char *uart = (char *)4096;

	/* very hacky, not recommended but good enough for testing */
	if (x == 0 && y == 2) {
		*uart = '0';
		*uart = '\n';
	}
	else
		*control = 0; /* go to sleep */

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

	volatile unsigned long *next = (unsigned long *)(((unsigned long)x << 32) | ((unsigned long)y << 48) + 128);
	*next = 1; /* wake next core */
	*control = 0; /* go to sleep again */
}
