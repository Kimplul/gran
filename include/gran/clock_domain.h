#ifndef EXSIM_CLOCK_DOMAIN_H
#define EXSIM_CLOCK_DOMAIN_H

#include <stdint.h>
#include <stdbool.h>

#include <gran/component.h>

typedef uint64_t tick;

struct clock_time {
	uint64_t s;
	uint64_t fs;
};

#define MAX_CLOCK (struct clock_time){-1, -1}

#define FS(x) (x)
#define PS(x) (1000ULL * FS(x))
#define NS(x) (1000ULL * PS(x))
#define US(x) (1000ULL * NS(x))
#define MS(x) (1000ULL * US(x))
#define SEC(x) (1000ULL * MS(x))

struct clock_domain;

struct clock_domain *create_clock_domain(tick ps);
void destroy_clock_domain(struct clock_domain *);

stat clock_domain_add(struct clock_domain *, struct component *);
stat clock_domain_tick(struct clock_domain *);

bool eq_time(struct clock_time a, struct clock_time b);
bool lt_time(struct clock_time a, struct clock_time b);
bool le_time(struct clock_time a, struct clock_time b);

struct clock_time max_time(struct clock_time a, struct clock_time b);
struct clock_time domain_time(struct clock_domain *);

#endif /* EXSIM_CLOCK_DOMAIN_H */
