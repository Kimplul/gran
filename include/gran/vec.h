/* SPDX-License-Identifier: copyleft-next-0.3.1 */

#ifndef VEC_H
#define VEC_H

#include <stddef.h>

struct vec {
	size_t n;
	size_t s;
	size_t ns;
	void *buf;
};

struct vec vec_create(size_t s);
void vec_destroy(struct vec *v);
void vec_reset(struct vec *v);

size_t vec_len(struct vec *v);
void *vec_at(struct vec *v, size_t i);
void *vec_back(struct vec *v);
void *vec_pop(struct vec *v);
void vec_append(struct vec *v, void *n);

typedef int (*vec_comp_t)(const void *, const void *);
void vec_sort(struct vec *v, vec_comp_t comp);

#define foreach_vec(iter, v) \
	for (size_t iter = 0; iter < vec_len(&v); ++iter)

#define vect_at(type, v, i) \
	*(type *)vec_at(&v, i)

#define vect_append(type, v, e) \
	vec_append(&v, (type *)(e))

#define vect_back(type, v) \
	*(type *)vec_back(&v)

#define vect_pop(type, v) \
	*(type *)vec_pop(&v)

#define vec_uninit(v) \
	(v.buf == NULL)

#endif /* VEC_H */
