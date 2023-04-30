#ifndef GRAN_ROOT_H
#define GRAN_ROOT_H

#include <gran/common.h>
#include <gran/clock_domain.h>

struct gran_root;

struct gran_root *create_root();
stat root_add_clock(struct gran_root *, struct clock_domain *);
stat root_run(struct gran_root *);
void destroy_root(struct gran_root *);

#endif /* GRAN_ROOT_H */
