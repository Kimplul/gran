#ifndef GRAN_ALLOC_IF_H
#define GRAN_ALLOC_IF_H

#include <stdint.h>

/* completely arbitrary values */
enum alloc_if_op {
	ALLOC_IF_NEW = 0x00,
	ALLOC_IF_RM = 0x40,
	ALLOC_IF_Q = 0x80,
};

enum alloc_if_ret {
	ALLOC_IF_OK,
	ALLOC_IF_ERR_SPACE,
	ALLOC_IF_ERR_RANGE,
};

struct alloc_if_new {
	uint64_t space, start, end;
};

struct alloc_if_rm {
	uint64_t space, start, end;
};

struct alloc_if_q {
	uint64_t space, addr;
};

struct alloc_if_r {
	enum alloc_if_ret r;
	uint64_t start, end;
};

#endif /* GRAN_ALLOC_IF_H */
