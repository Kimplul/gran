#include <stdio.h>
#include <gran/uart/simple_uart.h>

struct simple_uart {
	struct component component;

	struct component *send;
	struct packet pkt;
	bool busy;
};

static stat simple_uart_clock(struct simple_uart *uart)
{
	if (!uart->busy)
		return OK;

	stat r = SEND(uart, uart->send, uart->pkt);
	if (r == EBUSY)
		return OK;

	uart->busy = false;
	return OK;
}

static stat simple_uart_receive(struct simple_uart *uart,
                                struct component *from, struct packet pkt)
{
	if (uart->busy)
		return EBUSY;

	uart->busy = true;
	uart->send = from;
	uart->pkt = response(pkt);

	size_t size = packet_convsize(&pkt);
	if (size != 1) {
		set_flags(&uart->pkt, PACKET_ERROR);
		return OK;
	}

	putchar(packet_convu8(&pkt));
	fflush(stdout);
	set_flags(&uart->pkt, PACKET_DONE);
	return OK;
}

struct component *create_simple_uart()
{
	struct simple_uart *uart = calloc(1, sizeof(struct simple_uart));
	if (!uart)
		return NULL;

	uart->component.receive = (receive_callback)simple_uart_receive;
	uart->component.clock = (clock_callback)simple_uart_clock;
	return (struct component *)uart;
}
