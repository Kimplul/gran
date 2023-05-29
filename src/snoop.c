#include <stdlib.h>

#include <gran/snoop.h>

struct snoop {
	enum snoop_state state;
	uintptr_t addr;
	size_t size;
};

struct snoop *create_snoop(uintptr_t addr, size_t size)
{
	struct snoop *snoop = calloc(1, sizeof(struct snoop));

	snoop->state = SNOOP_UNANSWERED;
	snoop->addr = addr;
	snoop->size = size;
	return snoop;
}

enum snoop_state snoop_state(struct snoop *snoop)
{
	return snoop->state;
}

uintptr_t snoop_addr(struct snoop *snoop)
{
	return snoop->addr;
}

size_t snoop_size(struct snoop *snoop)
{
	return snoop->size;
}

void destroy_snoop(struct snoop *snoop)
{
	free(snoop);
}
