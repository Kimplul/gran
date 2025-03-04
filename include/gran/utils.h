#ifndef GRAN_UTILS_H
#define GRAN_UTILS_H

#include <stdbool.h>

/* assumes x != 0 */
static inline bool is_powerof2(unsigned long long x)
{
	return __builtin_popcountll(x) == 1;
}

/* assumes x != 0 */
static inline unsigned long long log2ull(unsigned long long x)
{
	return __builtin_ffsll(x) - 1;
}

#endif /* GRAN_UTILS_H */
