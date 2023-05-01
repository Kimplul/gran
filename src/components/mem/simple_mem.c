/* SPDX-License-Identifier: copyleft-next-0.3.1 */
/* Copyright 2023 Kim Kuparinen < kimi.h.kuparinen@gmail.com > */

#include <string.h>
#include <stdlib.h>

#include <gran/mem/simple_mem.h>

struct simple_mem {
	struct component component;
	size_t size;
	char buf[];
};

static stat simple_mem_write(struct simple_mem *mem, uintptr_t addr,
                             size_t size, char *buf)
{
	uintptr_t offset = addr % mem->size;
	if (offset + size > mem->size) {
		error("write outside memory");
		return ESIZE;
	}

	memcpy(mem->buf + offset, buf, size);
	return OK;
}

static stat simple_mem_read(struct simple_mem *mem, uintptr_t addr, size_t size,
                            char *buf)
{
	uintptr_t offset = addr % mem->size;
	if (offset + size > mem->size) {
		error("read outside memory");
		return ESIZE;
	}

	memcpy(buf, mem->buf + offset, size);
	return OK;
}

struct component *create_simple_mem(size_t size)
{
	struct simple_mem *new = calloc(1, sizeof(struct simple_mem) + size);
	if (!new)
		return NULL;

	new->size = size;
	new->component.write = (write_callback)simple_mem_write;
	new->component.read = (read_callback)simple_mem_read;
	return (struct component *)new;
}
