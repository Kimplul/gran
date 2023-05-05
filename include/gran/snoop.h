#ifndef GRAN_SNOOP_H
#define GRAN_SNOOP_H

#include <stdint.h>

enum snoop_state {
	SNOOP_UNANSWERED,
	SNOOP_ANSWERED,
};

struct snoop;

struct snoop *create_snoop(uintptr_t addr, size_t size);
void destroy_snoop(struct snoop *snoop);

enum snoop_state snoop_state(struct snoop *snoop);
uintptr_t snoop_addr(struct snoop *snoop);
size_t snoop_size(struct snoop *snoop);

#endif /* GRAN_SNOOP_H */
