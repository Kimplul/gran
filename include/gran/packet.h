#ifndef GRAN_PACKET_H
#define GRAN_PACKET_H

#include <stdint.h>
#include <stddef.h>

enum packet_state {
	PACKET_INIT,
	PACKET_FAILED,
	PACKET_SENT,
	PACKET_WAITING,
	PACKET_DONE,
};

enum packet_type {
	PACKET_READ,
	PACKET_WRITE,
	PACKET_SWAP,
	PACKET_SNOOP,
	PACKET_CTRL,
};

struct packet;

struct packet *create_packet(enum packet_type type, uintptr_t addr,
                             size_t size);
struct packet *create_packet_with(enum packet_type type, uintptr_t addr,
                                  size_t size, void *data);

enum packet_type packet_type(struct packet *pkt);
enum packet_state packet_state(struct packet *pkt);
void packet_set_state(struct packet *pkt, enum packet_state state);

size_t packet_size(struct packet *pkt);
uintptr_t packet_addr(struct packet *pkt);
void *packet_data(struct packet *pkt);

void destroy_packet(struct packet *pkt);

#endif /* GRAN_PACKET_H */
