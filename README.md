# gran

Gran is a toy granular full system simulator,
taking heavy inspiration from [gem5](https://www.gem5.org/).

Currently there are a couple example components, including
`simple_mem` for atomic memory accesses, `simple_bus` for non-coherent
atomic transfers between components and `simple_riscv32`, a simple
rv32i processor.

# Building

+ `make`

Add `RELEASE=1` to enable optimizations.
