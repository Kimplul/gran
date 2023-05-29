#include <gran/packet.h>
#include <stdlib.h>
#include <assert.h>

struct packet {
	enum packet_state state;
	enum packet_type type;

	uintptr_t addr;
	size_t size;
	void *data;
};


struct packet *create_packet(enum packet_type type, uintptr_t addr, size_t size)
{
	struct packet *pkt = malloc(sizeof(struct packet) + size);
	if (!pkt)
		return NULL;

	pkt->state = PACKET_INIT;
	pkt->type = type;
	pkt->addr = addr;
	pkt->size = size;
	pkt->data = pkt + 1;
	return pkt;
}

struct packet *create_packet_with(enum packet_type type, uintptr_t addr,
                                  size_t size, void *data)
{
	struct packet *pkt = malloc(sizeof(struct packet));
	if (!pkt)
		return NULL;

	pkt->state = PACKET_INIT;
	pkt->type = type;
	pkt->addr = addr;
	pkt->size = size;
	pkt->data = data;
	return pkt;
}

struct packet *reuse_packet(struct packet *pkt, enum packet_type type,
                            uintptr_t addr, size_t size, void *data)
{
	assert(pkt->state == PACKET_DONE);
	pkt->state = PACKET_INIT;
	pkt->type = type;
	pkt->addr = addr;
	pkt->size = size;
	pkt->data = data;
	return pkt;
}

enum packet_type packet_type(struct packet *pkt)
{
	return pkt->type;
}

enum packet_state packet_state(struct packet *pkt)
{
	return pkt->state;
}

void packet_set_state(struct packet *pkt, enum packet_state state)
{
	pkt->state = state;
}

size_t packet_size(struct packet *pkt)
{
	return pkt->size;
}

uintptr_t packet_addr(struct packet *pkt)
{
	return pkt->addr;
}

void *packet_data(struct packet *pkt)
{
	return pkt->data;
}

void destroy_packet(struct packet *pkt)
{
	free(pkt);
}
