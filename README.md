# gran

Gran is a toy granular full system simulator, taking heavy inspiration from
[gem5](https://www.gem5.org/).

Currently there are a couple example components, including `simple_mem` for
atomic memory accesses, `simple_bus` for non-coherent atomic transfers between
components and `simple_riscv32`, a simple rv32i processor.

# Building

+ `make`

Add `RELEASE=1` to enable optimizations.

# manycore

Idea: Have a grid of x/y width, where each node has three 'axes', i.e. if the
address matches the node, pass the message to whatever is at that node (think of
it like a miniature system I suppose?), otherwise check x, send it in that
direction, otherwise check y, send it in that direction, otherwise check if
message should be transcended, i.e. sent to some other cluster.

Each address is 64 bits, split in half. The lower 32 bits is the offset within
the node, and the upper 32 bits are split (again) in twine, something like x1 y1
x2 y2 where each x or y is eight bits, specifying a node in an x/y grid. x2/y2
specifies the lower cluster, with x1/y1 upper cluster. The idea is that a
complete system would consist of cards that contain one cluster, clustered
together. So a 'supercomputer' could be up to 65536 cards, with each card having
a cluster of 65536 nodes. Each node is a compute unit, probably a single core,
possibly with associated memory or devices like UART, whatever.

Currently I'm imagining that each node also has a control region that can be
written to with commands like 'drop all cache entries that containt address xyz'
or 'wake up' (to enable userspace mutexes, that would be massively cool)

The current focus would be to enable lightweight userspaces, full-blown
hardware-assisted virtual memory is costly to implement (as in takes up too much
space in hardware), but I'd still want to be able to run a command line and
generic programs on the system. One idea would be a capability-based system,
sort of like CHERIoT, though it has some drawbacks like having to keep track of
the valid capabilities in separate memory or as part of ECC.

Another approach that I'm currently thinking of is having a *very* simple
region-based memory protection scheme, where instead of paging we have a TLB of,
dunno, 64 regions that the core is allowed to access. If the core tries to
access a region outside of those regions, the kernel is trapped to and can then
insert the requested area into the regions or kill the process or whatever. This
is *similar* to virtual memory in other systems (particularly MIPS), but would
be range-based rather than page-based. If I wanted to use OpenASIP, I probably
wouldn't be able to use interrupts, and we would either need a separate
coprocessor to handle these kinds of exceptions or implement a tree walker in
hardware (unsure how much extra area it would take)
