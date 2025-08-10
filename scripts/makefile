# this could be done better
RELEASE		?= 0
OPTFLAGS	!= [ "$(RELEASE)" != "0" ] \
			&& echo "-O2 -flto=auto" \
			|| echo "-O0"

DEBUG		?= 1
DEBUGFLAGS	!= [ "$(DEBUG)" != "0" ] \
			&& echo "-DDEBUG=1" \
			|| echo "-DNDEBUG=1"

ASSERT		?= 1
ASSERTFLAGS	!= [ "$(ASSERT)" != "0" ] \
			&& echo "-DASSERT=1" \
			|| echo

DEP_FLAGS	= -MT $@ -MMD -MP -MF $@.d
LINT_FLAGS	:= -fsyntax-only
PREPROCESS	:= -E

LLVM		?= 0
BUILD		:= build

all: libgran.a

# default values, overwrite if/when needed
CROSS_COMPILE	:=

OBJCOPY		!= [ "$(LLVM)" != "0" ] \
			&& echo llvm-objcopy \
			|| echo $(CROSS_COMPILE)objcopy

COMPILER	!= [ -n "$(CROSS_COMPILE)" ]						\
			&& {								\
				[ "$(LLVM)" != "0" ]					\
					&& echo clang --target="$(CROSS_COMPILE)"	\
					|| echo $(CROSS_COMPILE)gcc			\
					;						\
			}								\
			|| echo $(CC)


OBFLAGS		:= -g
WARNFLAGS	:= -Wall -Wextra

COMPILE_FLAGS	:= $(CFLAGS) $(WARNFLAGS) $(OPTFLAGS) $(OBFLAGS) $(ASSERTFLAGS) \
		   $(DEBUGFLAGS)

INCLUDE_FLAGS	:= -I include -I deps/conts/include

LINT		= $(COMPILER) \
		  $(COMPILE_FLAGS) $(LIN_TFLAGS) $(INCLUDE_FLAGS)

UBSAN		?= 0
SANITIZE_FLAGS	!= [ "$(UBSAN)" != "0" ] \
			&& echo -fsanitize=undefined \
			|| echo

# don't use dep trick for tests
COMPILE_TEST	:= $(COMPILER) $(COMPILE_FLAGS) \
		  $(INCLUDE_FLAGS) $(SANITIZE_FLAGS)

# dep flags use delayed expansion trick so we can't use := here
COMPILE_GRAN	= $(COMPILER) $(COMPILE_FLAGS) \
		  $(DEP_FLAGS) $(INCLUDE_FLAGS) $(SANITIZE_FLAGS)

TESTS		!= [ -n "$(CHECK)" ] && echo "$(CHECK)" || echo check

-include deps.mk

libgran.a: $(GRAN_OBJS)
	$(AR) rcs $@ $(GRAN_OBJS)


# might lint some common things twice
.PHONY:
lint: $(GRAN_LINTS)

.PHONY: check
check:
	$(MAKE) -f scripts/makefile.tests -k $(TESTS) COMPILE_TEST="$(COMPILE_TEST)"
