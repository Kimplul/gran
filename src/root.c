#include <stdlib.h>

#include <gran/root.h>
#include <gran/clock_domain.h>

#define MAX_DOMAINS 512

struct gran_root {
	size_t num_domains;
	struct clock_domain *(domains[MAX_DOMAINS]);
};

struct gran_root *create_root()
{
	return calloc(1, sizeof(struct gran_root));
}

stat root_add_clock(struct gran_root *root, struct clock_domain *clk)
{
	if (root->num_domains == MAX_DOMAINS) {
		error("too many clock domains");
		return ESIZE;
	}

	root->domains[root->num_domains++] = clk;
	return OK;
}

static struct clock_domain *most_delayed_domain(struct gran_root *root)
{
	struct clock_domain *min = root->domains[0];

	for (size_t i = 1; i < root->num_domains; ++i) {
		struct clock_domain *cur = root->domains[i];
		if (lt_time(domain_time(cur), domain_time(min)))
			min = cur;
	}

	return min;
}

stat root_run(struct gran_root *root)
{
	if (root->num_domains == 0) {
		info("no clock domains added to root, exiting");
		return OK;
	}

	stat ret = OK;
	while (ret == OK)
		ret = clock_domain_tick(most_delayed_domain(root));

	if (ret == DONE)
		return OK;

	return ret;
}

void destroy_root(struct gran_root *root)
{
	for (size_t i = 0; i < root->num_domains; ++i)
		destroy_clock_domain(root->domains[i]);

	free(root);
}
