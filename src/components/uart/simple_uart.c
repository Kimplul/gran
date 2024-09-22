#include <stdio.h>
#include <gran/uart/simple_uart.h>

struct simple_uart {
	struct component component;
};

static stat simple_uart_write(struct simple_uart *uart, struct packet *pkt)
{
	(void)uart;
	size_t size = packet_size(pkt);
	if (size != 1)
		return EBUS;

	putchar(*(uint8_t *)packet_data(pkt));
	packet_set_state(pkt, PACKET_DONE);
	return OK;
}

struct component *create_simple_uart()
{
	struct simple_uart *uart = calloc(1, sizeof(struct simple_uart));
	if (!uart)
		return NULL;

	uart->component.write = (write_callback)simple_uart_write;
	return (struct component *)uart;
}
