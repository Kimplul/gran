#ifndef GRAN_SIMT_RISCV64_H
#define GRAN_SIMT_RISCV64_H

#include <stdint.h>
#include <gran/component.h>

struct simt_riscv64_conf {
        uint64_t data_rcv;
        uint64_t inst_rcv;
        uint64_t start_pc;
        uint64_t num_lanes;
};

struct component *create_simt_riscv64(
                struct simt_riscv64_conf conf,
                struct component *imem,
                struct component *dmem);

struct component *simt_riscv64_data_intf(struct component *c);
struct component *simt_riscv64_inst_intf(struct component *c);

void simt_riscv64_set_reg(struct component *c, size_t lane, size_t reg, uint64_t val);

#endif /* GRAN_SIMT_RISCV64_H */
