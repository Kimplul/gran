.PHONY: all
all: setup
	$(MAKE) -f scripts/makefile

# this kicks all unrecognised targets to the client script.
# note that trying to compile individual files, e.g.
#
#	make kernel.elf
#
# will not work, you would need
#
#	make -f scripts/makefile kernel.elf
#
# instead
.DEFAULT: setup
	$(MAKE) -f scripts/makefile $<

.PHONY: analyze
analyze: setup
	CFLAGS="$$CFLAGS -fanalyzer" SKIP_ANALYZER='-fno-analyzer' $(MAKE)

.PHONY: setup
setup:
	@echo -n > deps.mk
	@./scripts/gen-deps -p GRAN -c COMPILE_GRAN -b gran "$(GRAN_SOURCES)"

CLEANUP		:= build reports deps.mk libgran.a
CLEANUP_CMD	:=
GRAN_SOURCES	:=

include src/source.mk

.PHONY: check
check: all
	$(MAKE) -f scripts/makefile -k check CHECK=$(CHECK)

.PHONY: format
format:
	find src include -iname '*.[ch]' |\
		xargs uncrustify -c uncrustify.conf --no-backup -F -

.PHONY: license
license:
	find src include -iname '*.[ch]' |\
		xargs ./scripts/license

.PHONY: docs
docs:
	find src include -iname '*.[ch]' |\
		xargs ./scripts/warn-undocumented
	doxygen docs/doxygen.conf

RM	= rm

.PHONY: clean
clean:
	$(RM) -rf $(CLEANUP)

.PHONY: clean_docs
clean_docs:
	$(RM) -rf docs/output

.PHONY: clean_all
clean_all: clean clean_docs
