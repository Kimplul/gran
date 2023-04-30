#ifndef GRAN_COMMON_H
#define GRAN_COMMON_H

#include <stdio.h>

typedef enum {
	OK = 0,
	DONE,
	EBUS,
	EBUSY,
	EMEM,
	ENOMEM,
	EEXISTS,
	ENOSUCH,
	ESIZE,
} stat;

#define error(x, ...) \
	do {fprintf(stderr, "error: " x "\n",##__VA_ARGS__);} while(0)

#define warn(x, ...) \
	do {fprintf(stderr, "warn: " x "\n",##__VA_ARGS__);} while(0)

#define info(x, ...) \
	do {fprintf(stderr, "info: " x "\n",##__VA_ARGS__);} while(0)

#if DEBUG
#define debug(x, ...) \
	do {fprintf(stderr, "debug: " x "\n",##__VA_ARGS__);} while(0)
#else
#define debug(x, ...)
#endif

#endif /* GRAN_COMMON_H */
