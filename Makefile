DO	!= echo -n > deps.mk

DEBUGFLAGS	!= [ $(RELEASE) ] && echo "-flto=auto -O2 -g -DNODEBUG" || echo "-O0 -g -DDEBUG"
CFLAGS		= -Wall -Wextra -fopenmp
DEPFLAGS	= -MT $@ -MMD -MP -MF $@.d
INCLUDEFLAGS	= -Iinclude
COMPILEFLAGS	=
LINKFLAGS	=

all: gran

# default values
CROSS_COMPILE	?=

# common programs
CC		= gcc

SOURCES		:=

include src/source.mk

COMPILE		= $(CROSS_COMPILE)$(CC) $(DEBUGFLAGS)\
		  $(CFLAGS) $(DEPFLAGS) $(COMPILEFLAGS) $(INCLUDEFLAGS)

OBJS		!= ./scripts/gen-deps --sources "$(SOURCES)"
MAIN_OBJ	!= ./scripts/gen-deps --sources "$(MAIN_SRC)"

include tests/source.mk
include deps.mk

.PHONY: format
format:
	@cd src; find . -iname '*.[ch]' |\
		xargs -n 10 -P 0 uncrustify -c ../uncrustify.conf --no-backup -F -
	@cd include; find . -iname '*.[ch]' |\
		xargs -n 10 -P 0 uncrustify -c ../uncrustify.conf --no-backup -F -

.PHONY: license
license:
	@find . -iname '*.[ch]' |\
		xargs -n 10 -P 0 ./scripts/license

.PHONY: docs
docs:
	@./scripts/warn-undocumented
	@doxygen docs/doxygen.conf

.PHONY: check
check: $(TEST_PROGS)
	./tests/check.sh

gran: $(MAIN_OBJ) $(OBJS)
	$(COMPILE) $(MAIN_OBJ) $(OBJS) -o $@

.PHONY: clean
clean:
	@$(RM) -r build gran deps.mk

.PHONY: clean_docs
clean_docs:
	@$(RM) -r docs/output
