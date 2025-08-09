#include <gran/common.h>

stat place_reg(struct reg *r, struct packet pkt)
{
	if (r->busy)
		return EBUSY;

	r->pkt = pkt;
	r->busy = true;
	return OK;
}

stat copy_reg(struct reg *r, struct reg *s)
{
	assert(s->busy);
	if (r->busy)
		return EBUSY;

	r->pkt = s->pkt;
	r->busy = true;
	s->busy = false;
	return OK;
}

void propagate(struct reg *out,
                      size_t count, struct reg *in[static count],
                      bool (*sel)(struct reg *r, void *data), void *data)
{
	struct reg *r = NULL;
	for (size_t i = 0; i < count; ++i) {
		if (!in[i] || !in[i]->busy)
			continue;

		if (!sel(in[i], data))
			continue;

		if (!r || r->pkt.timestamp > in[i]->pkt.timestamp)
			r = in[i];
	}

	if (!r)
		return;

	copy_reg(out, r);
}
