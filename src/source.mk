include src/components/source.mk

# everything except main
SRC_LOCAL != echo src/*.c | sed 's|src/main.c||g'
SOURCES += $(SRC_LOCAL)
MAIN_SRC = src/main.c
