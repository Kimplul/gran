/* SPDX-License-Identifier: copyleft-next-0.3.1 */
/* Copyright 2023 Kim Kuparinen < kimi.h.kuparinen@gmail.com > */

#ifndef GRAN_PACKET_H
#define GRAN_PACKET_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>

enum packet_flags {
	PACKET_READ   = (1 << 0),
	PACKET_WRITE  = (1 << 1),
	PACKET_ATOMIC = (1 << 2),
	PACKET_ERROR  = (1 << 3),
	PACKET_DONE   = (1 << 4)
};

struct packet {
	uint64_t from;
	uint64_t to;
	uint64_t mask;
	uint64_t timestamp;
	uint8_t data[64];
	enum packet_flags flags;
};

static inline struct packet response(struct packet pkt)
{
	uint64_t from = pkt.from;
	uint64_t to = pkt.to;
	pkt.from = to;
	pkt.to = from;
	return pkt;
}

static inline void set_flags(struct packet *pkt, enum packet_flags flags)
{
	pkt->flags |= flags;
}

static inline void clear_flags(struct packet *pkt, enum packet_flags flags)
{
	pkt->flags &= ~flags;
}

static inline bool is_set(struct packet *pkt, enum packet_flags flags)
{
	return pkt->flags & flags;
}

static inline uint64_t packet_align(uint64_t addr)
{
	return addr & ~(64 - 1);
}

static inline uint64_t packet_mask(uint64_t addr, uint64_t size)
{
	assert(size <= 64);
	uint64_t mask = size == 64 ? ((uint64_t)-1) : (((uint64_t)1 << size) - 1);
	uint64_t aligned = packet_align(addr);
	uint64_t diff = addr - aligned;

	assert((diff == 0) || ((mask >> (64 - diff)) == 0));
	mask <<= diff;
	return mask;
}

static inline uint64_t packet_convsize(struct packet *pkt)
{
	int start = __builtin_ffsll(pkt->mask);
	if (start == 0)
		return 0;

	switch ((pkt->mask >> (start - 1)) & 0xff) {
	case 0x01: return 1;
	case 0x03: return 2;
	case 0x0f: return 4;
	case 0xff: return 8;
	default: abort();
	}

	return 0;
}

static inline uint64_t packet_convto(struct packet *pkt)
{
	int start = __builtin_ffsll(pkt->mask);
	if (start == 0)
		return pkt->to;

	return pkt->to + (start - 1);
}

static inline uint64_t packet_convfrom(struct packet *pkt)
{
	int start = __builtin_ffsll(pkt->mask);
	if (start == 0)
		return pkt->from;

	return pkt->from + (start - 1);
}

static inline int64_t packet_convi8(struct packet *pkt)
{
	int start = __builtin_ffsll(pkt->mask);
	assert(start != 0);

	return pkt->data[start - 1];
}

static inline uint8_t packet_convu8(struct packet *pkt)
{
	int start = __builtin_ffsll(pkt->mask);
	assert(start != 0);

	return pkt->data[start - 1];
}

static inline int16_t packet_convi16(struct packet *pkt)
{
	int start = __builtin_ffsll(pkt->mask);
	assert(start != 0);

	int16_t res = 0;
	memcpy(&res, pkt->data + (start - 1), 2);
	return res;
}

static inline uint16_t packet_convu16(struct packet *pkt)
{
	int start = __builtin_ffsll(pkt->mask);
	assert(start != 0);

	uint16_t res = 0;
	memcpy(&res, pkt->data + (start - 1), 2);
	return res;
}

static inline int32_t packet_convi32(struct packet *pkt)
{
	int start = __builtin_ffsll(pkt->mask);
	assert(start != 0);

	int32_t res = 0;
	memcpy(&res, pkt->data + (start - 1), 4);
	return res;
}

static inline uint32_t packet_convu32(struct packet *pkt)
{
	int start = __builtin_ffsll(pkt->mask);
	assert(start != 0);

	uint32_t res = 0;
	memcpy(&res, pkt->data + (start - 1), 4);
	return res;
}

static inline int64_t packet_convi64(struct packet *pkt)
{
	int start = __builtin_ffsll(pkt->mask);
	assert(start != 0);

	int64_t res = 0;
	memcpy(&res, pkt->data + (start - 1), 8);
	return res;
}

static inline uint64_t packet_convu64(struct packet *pkt)
{
	int start = __builtin_ffsll(pkt->mask);
	assert(start != 0);

	uint64_t res = 0;
	memcpy(&res, pkt->data + (start - 1), 8);
	return res;
}

static inline struct packet create_packet(uint64_t from, uint64_t to, uint64_t size, void *data, enum packet_flags flags)
{
	assert(size <= 64);
	assert(packet_align(from) == from);

	uint64_t aligned = packet_align(to);
	uint64_t mask = packet_mask(to, size);
	struct packet pkt = (struct packet){
		.timestamp = 0,
		.from = from,
		.to = aligned,
		.mask = mask,
		.data = {},
		.flags = flags
	};

	if (is_set(&pkt, PACKET_WRITE))
		memcpy(pkt.data + (to - aligned), data, size);

	return pkt;
}

static inline void checked_copyto(struct packet *pkt, uint8_t *data)
{
	for (size_t i = 0; i < 64; ++i) {
		if (pkt->mask & ((uint64_t)1 << i))
			pkt->data[i] = data[i];
	}
}

static inline void checked_copyfrom(struct packet *pkt, uint8_t *data)
{
	for (size_t i = 0; i < 64; ++i) {
		if (pkt->mask & ((uint64_t)1 << i))
			data[i] = pkt->data[i];
	}
}

#endif /* GRAN_PACKET_H */
