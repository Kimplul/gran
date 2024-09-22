/* SPDX-License-Identifier: copyleft-next-0.3.1 */

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <gran/vec.h>

struct vec vec_create(size_t ns)
{
	return (struct vec) {
		       .n = 0,
		       .s = 1,
		       .ns = ns,
		       .buf = malloc(ns),
	};
}

size_t vec_len(struct vec *v)
{
	return v->n;
}

void *vec_at(struct vec *v, size_t i)
{
	assert(i < v->n && "out of vector bounds");
	return v->buf + i * v->ns;
}

void *vec_back(struct vec *v)
{
	assert(v->n);
	return v->buf + (v->n - 1) * v->ns;
}

void *vec_pop(struct vec *v)
{
	assert(v->n && "attempting to pop empty vector");
	v->n--;
	return v->buf + v->n * v->ns;
}

void vec_append(struct vec *v, void *n)
{
	v->n++;
	if (v->n >= v->s) {
		v->s *= 2;
		v->buf = realloc(v->buf, v->s * v->ns);
	}

	void *p = vec_at(v, v->n - 1);
	memcpy(p, n, v->ns);
}

void vec_reset(struct vec *v)
{
	v->n = 0;
}

void vec_destroy(struct vec *v) {
	free(v->buf);
}

void vec_sort(struct vec *v, vec_comp_t comp)
{
	qsort(v->buf, v->n, v->ns, comp);
}
