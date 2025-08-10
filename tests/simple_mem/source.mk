SIMPLE_MEM		:= tests/simple_mem
SIMPLE_MEM_NO_BUS	:= $(SIMPLE_MEM)/traffic_gen.c $(SIMPLE_MEM)/no_bus.c
SIMPLE_MEM_BUS		:= $(SIMPLE_MEM)/traffic_gen.c $(SIMPLE_MEM)/bus.c
SIMPLE_MEM_MANY		:= $(SIMPLE_MEM)/traffic_gen.c $(SIMPLE_MEM)/many.c

TESTS += $(SIMPLE_MEM)/no_bus $(SIMPLE_MEM)/bus $(SIMPLE_MEM)/many

build/$(SIMPLE_MEM):
	mkdir -p $@

$(SIMPLE_MEM)/no_bus: build/$(SIMPLE_MEM) $(SIMPLE_MEM_NO_BUS) libgran.a
	$(COMPILE_TEST) $(SIMPLE_MEM_NO_BUS) libgran.a -o build/$@
	./scripts/gen-report -d build $@

$(SIMPLE_MEM)/bus: build/$(SIMPLE_MEM) $(SIMPLE_MEM_BUS) libgran.a
	$(COMPILE_TEST) $(SIMPLE_MEM_BUS) libgran.a -o build/$@
	./scripts/gen-report -d build $@

$(SIMPLE_MEM)/many: build/$(SIMPLE_MEM) $(SIMPLE_MEM_MANY) libgran.a
	$(COMPILE_TEST) $(SIMPLE_MEM_MANY) libgran.a -o build/$@
	./scripts/gen-report -d build $@
