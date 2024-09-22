/* SPDX-License-Identifier: copyleft-next-0.3.1 */
/* Copyright 2023 Kim Kuparinen < kimi.h.kuparinen@gmail.com > */

#include <byteswap.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include <gran/cpu/riscv/simple_riscv64.h>

struct simple_rv64_ldst {
	struct packet *pkt;
	uint32_t reg;
	bool u;
};

struct simple_riscv64 {
	struct component component;

	struct component *imem;
	struct component *dmem;

	struct simple_rv64_ldst dls;
	struct simple_rv64_ldst ils;

	/* have to be careful with x0 */
	uint64_t regs[32];
	uint64_t pc;
};

/* big endian format, going from smallest to highest address.
 * Easy to get confused  */
struct rtype {
	uint32_t op : 7;
	uint32_t rd : 5;
	uint32_t funct3 : 3;
	uint32_t rs1 : 5;
	uint32_t rs2 : 5;
	uint32_t funct7 : 7;
};

struct itype {
	uint32_t op : 7;
	uint32_t rd : 5;
	uint32_t funct3 : 3;
	uint32_t rs1 : 5;
	uint32_t imm : 12;
};

struct stype {
	uint32_t op : 7;
	uint32_t imm0 : 5;
	uint32_t funct3 : 3;
	uint32_t rs1 : 5;
	uint32_t rs2 : 5;
	uint32_t imm1 : 7;
};

struct btype {
	uint32_t op : 7;
	uint32_t imm0 : 1;
	uint32_t imm1 : 4;
	uint32_t funct3 : 3;
	uint32_t rs1 : 5;
	uint32_t rs2 : 5;
	uint32_t imm2 : 6;
	uint32_t imm3 : 1;
};

struct utype {
	uint32_t op : 7;
	uint32_t rd : 5;
	uint32_t imm : 20;
};

struct jtype {
	uint32_t op : 7;
	uint32_t rd : 5;
	uint32_t imm0 : 8;
	uint32_t imm1 : 1;
	uint32_t imm2 : 10;
	uint32_t imm3 : 1;
};

enum opcode {
	LOAD      = 0b0000011,
	LOAD_FP   = 0b0000111,
	MISC_MEM  = 0b0001111,
	OP_IMM    = 0b0010011,
	AUIPC     = 0b0010111,
	OP_IMM_32 = 0b0011011,

	STORE     = 0b0100011,
	STORE_FP  = 0b0100111,
	AMO       = 0b0101111,
	OP        = 0b0110011,
	LUI       = 0b0110111,
	OP_32     = 0b0111011,

	MADD     = 0b1000011,
	MSUB     = 0b1000111,
	NMSUB    = 0b1001011,
	NMADD    = 0b1001111,
	OP_FP    = 0b1010011,

	BRANCH   = 0b1100011,
	JALR     = 0b1100111,
	JAL      = 0b1101111,
	SYSTEM   = 0b1110011,
};

union rv_insn {
	struct rtype rtype;
	struct itype itype;
	struct stype stype;
	struct btype btype;
	struct utype utype;
	struct jtype jtype;
	uint32_t val;
};

static uint64_t get_reg(struct simple_riscv64 *cpu, size_t i)
{
	assert(i < 32);

	if (i == 0)
		return 0;

	return cpu->regs[i];
}

static void set_reg(struct simple_riscv64 *cpu, size_t i, uint64_t v)
{
	assert(i < 32);

	if (i == 0)
		return;

	cpu->regs[i] = v;
}

#define EXTEND_IMM12(x) ((int32_t)((x) << 20) >> 20)
#define EXTEND_IMM20(x) ((int32_t)((x) << 12) >> 12)
#define SHAMT(x) ((x) & 0b11111)

static stat op_imm(struct simple_riscv64 *cpu, union rv_insn insn)
{
	uint64_t dst = 0;
	uint64_t src = get_reg(cpu, insn.itype.rs1);

	uint64_t imm = EXTEND_IMM12(insn.itype.imm);

	switch (insn.rtype.funct3) {
	/* ADDI  */
	case 0b000: dst = src + imm; break;
	/* SLTI  */
	case 0b010: dst = (int64_t)src < (int64_t)imm; break;
	/* SLTIU */
	case 0b011: dst = src < imm; break;
	/* ANDI  */
	case 0b111: dst = src & imm; break;
	/* ORI   */
	case 0b110: dst = src | imm; break;
	/* XORI  */
	case 0b100: dst = src ^ imm; break;
	/* SLLI  */
	case 0b001: dst = src << SHAMT(imm); break;

	/* SRLI / SRAI */
	case 0b101:
		if (imm & ~0b11111)     /* SRLI */
			dst = src >> SHAMT(imm);
		else     /* SRAI */
			dst = (int64_t)src >> SHAMT(imm);
		break;

	default:
		error("unknown OP-IMM instruction: %x", insn.itype.funct3);
		return ENOSUCH;
	}

	set_reg(cpu, insn.itype.rd, dst);
	cpu->pc += 4;
	return OK;
}

static stat op_imm_32(struct simple_riscv64 *cpu, union rv_insn insn)
{
	uint64_t dst = 0;
	uint64_t src = get_reg(cpu, insn.itype.rs1);

	uint64_t imm = EXTEND_IMM12(insn.itype.imm);

	switch (insn.rtype.funct3) {
	/* ADDIW  */
	case 0b000: dst = src + imm; break;
	/* SLTI  */
	case 0b010: dst = (int64_t)src < (int64_t)imm; break;
	/* SLTIU */
	case 0b011: dst = src < imm; break;
	/* ANDI  */
	case 0b111: dst = src & imm; break;
	/* ORI   */
	case 0b110: dst = src | imm; break;
	/* XORI  */
	case 0b100: dst = src ^ imm; break;
	/* SLLIW  */
	case 0b001: dst = src << SHAMT(imm); break;

	/* SRLIW / SRAIW */
	case 0b101:
		if (imm & ~0b11111)     /* SRLI */
			dst = src >> SHAMT(imm);
		else     /* SRAI */
			dst = (int64_t)src >> SHAMT(imm);
		break;

	default:
		error("unknown OP-IMM instruction: %x", insn.itype.funct3);
		return ENOSUCH;
	}

	set_reg(cpu, insn.itype.rd, dst);
	cpu->pc += 4;
	return OK;
}

static stat lui(struct simple_riscv64 *cpu, union rv_insn insn)
{
	set_reg(cpu, insn.utype.rd, insn.utype.imm << 12);
	cpu->pc += 4;
	return OK;
}

static stat auipc(struct simple_riscv64 *cpu, union rv_insn insn)
{
	uint64_t res = cpu->pc + (insn.utype.imm << 12);
	set_reg(cpu, insn.utype.rd, res);
	cpu->pc += 4;
	return OK;
}

static stat op(struct simple_riscv64 *cpu, union rv_insn insn)
{
	uint64_t dst = 0;
	uint64_t src1 = get_reg(cpu, insn.rtype.rs1);
	uint64_t src2 = get_reg(cpu, insn.rtype.rs2);

	switch (insn.rtype.funct3) {
	/* ADD/SUB */
	case 0b000:
		if (insn.rtype.funct7) /* SUB */
			dst = src1 - src2;
		else /* ADD */
			dst = src1 + src2;
		break;

	/* SLT */
	case 0b010: dst = (int64_t)src1 < (int64_t)src2; break;
	/* SLTU */
	case 0b011: dst = src1 < src2; break;
	/* AND */
	case 0b111: dst = src1 & src2; break;
	/* OR */
	case 0b110: dst = src1 | src2; break;
	/* XOR */
	case 0b100: dst = src1 ^ src2; break;
	/* SLL */
	case 0b001: dst = src1 << src2; break;
	/* SRL/SRA */
	case 0b101:
		if (insn.rtype.funct7)     /* SRL */
			dst = src1 >> src2;
		else     /* SRA */
			dst = (int64_t)src1 >> src2;
		break;

	default:
		error("unknown OP instruction: %x", insn.rtype.funct3);
		return ENOSUCH;
	}

	set_reg(cpu, insn.rtype.rd, dst);
	cpu->pc += 4;
	return OK;
}

#define JTYPE_IMM(insn)                        \
	EXTEND_IMM20((insn.jtype.imm3 << 20)   \
	             | (insn.jtype.imm2 << 1)  \
	             | (insn.jtype.imm1 << 11) \
	             | (insn.jtype.imm0 << 12))

static stat jal(struct simple_riscv64 *cpu, union rv_insn insn)
{
	/** @todo generate exception on unaligned jumps */
	int64_t imm = JTYPE_IMM(insn);
	set_reg(cpu, insn.jtype.rd, cpu->pc + 4);
	cpu->pc += imm;
	return OK;
}

static stat jalr(struct simple_riscv64 *cpu, union rv_insn insn)
{
	int64_t src = get_reg(cpu, insn.itype.rs1);
	set_reg(cpu, insn.itype.rd, cpu->pc + 4);
	cpu->pc += src + EXTEND_IMM12(insn.itype.imm);
	return OK;
}

#define BTYPE_IMM(insn)                       \
	EXTEND_IMM12((insn.btype.imm3 << 12)  \
	             | (insn.btype.imm2 << 5) \
	             | (insn.btype.imm1 << 1) \
	             | (insn.btype.imm0 << 11))

static stat branch(struct simple_riscv64 *cpu, union rv_insn insn)
{
	uint64_t src1 = get_reg(cpu, insn.btype.rs1);
	uint64_t src2 = get_reg(cpu, insn.btype.rs2);
	int64_t offset = BTYPE_IMM(insn);

	switch (insn.btype.funct3) {
	/* BEQ */
	case 0b000:
		if (src1 == src2) {
			cpu->pc += offset;
			return OK;
		}
		break;

	/* BNE */
	case 0b001:
		if (src1 != src2) {
			cpu->pc += offset;
			return OK;
		}
		break;

	/* BLT */
	case 0b100:
		if ((int64_t)src1 < (int64_t)src2) {
			cpu->pc += offset;
			return OK;
		}
		break;

	/* BLTU */
	case 0b110:
		if (src1 < src2) {
			cpu->pc += offset;
			return OK;
		}
		break;

	/* BGE */
	case 0b101:
		if ((int64_t)src1 >= (int64_t)src2) {
			cpu->pc += offset;
			return OK;
		}
		break;

	/* BGEU */
	case 0b111:
		if (src1 >= src2) {
			cpu->pc += offset;
			return OK;
		}
		break;

	default:
		error("unknown BRANCH instruction %x", insn.btype.funct3);
		return ENOSUCH;
	}

	cpu->pc += 4;
	return OK;
}

static stat load(struct simple_riscv64 *cpu, union rv_insn insn)
{
	int64_t imm = EXTEND_IMM12(insn.itype.imm);
	int64_t base = get_reg(cpu, insn.itype.rs1);

	int64_t addr = base + imm;
	int64_t size = 0;
	bool u = false;

	// assume little endian for now
	switch (insn.itype.funct3) {
	/* LB/LBU */
	case 0b100: u = true; /* fallthrough */
	case 0b000: size = 1; break;
	/* LH/LHU */
	case 0b101: u = true; /* fallthrough */
	case 0b001: size = 2; break;
	/* LW */
	case 0b010: size = 4; break;
	case 0b011: size = 8; break;
	default:
		error("unknown LOAD width %x", insn.btype.funct3);
		return ENOSUCH;
	}

	struct packet *pkt = create_packet(PACKET_READ, addr, size);
	if (!pkt)
		return EMEM;

	cpu->dls = (struct simple_rv64_ldst){pkt, insn.itype.rd, u};
	stat ret = read(cpu->dmem, pkt);
	if (ret)
		return ret;

	cpu->pc += 4;
	return OK;
}

#define STYPE_IMM(insn)                     \
	EXTEND_IMM12((insn.stype.imm1 << 5) \
	             | (insn.stype.imm0))

static stat store(struct simple_riscv64 *cpu, union rv_insn insn)
{
	int64_t imm = STYPE_IMM(insn);
	int64_t base = get_reg(cpu, insn.stype.rs1);
	int64_t addr = base + imm;

	uint64_t src = get_reg(cpu, insn.stype.rs2);

	switch (insn.stype.funct3) {
	/* SB */
	case 0b000: {
		cpu->dls.pkt = create_packet(PACKET_WRITE, addr, 1);
		*(uint8_t *)packet_data(cpu->dls.pkt) = src;
		break;
	}
	/* SH */
	case 0b001: {
		cpu->dls.pkt = create_packet(PACKET_WRITE, addr, 2);
		*(uint16_t *)packet_data(cpu->dls.pkt) = src;
		break;
	}
	/* SW */
	case 0b010: {
		cpu->dls.pkt = create_packet(PACKET_WRITE, addr, 4);
		*(uint32_t *)packet_data(cpu->dls.pkt) = src;
		break;
	}
	/* SD */
	case 0b011: {
		cpu->dls.pkt = create_packet(PACKET_WRITE, addr, 8);
		*(uint64_t *)packet_data(cpu->dls.pkt) = src;
		break;
	}
	default:
		error("unknown width of STORE %x", insn.stype.funct3);
		return ENOSUCH;
	}

	cpu->pc += 4;
	return write(cpu->dmem, cpu->dls.pkt);
}

static void finalize_ld(struct simple_riscv64 *cpu)
{
	struct simple_rv64_ldst ld = cpu->dls;

	uint64_t val = 0;
	void *data = packet_data(ld.pkt);
	switch (packet_size(ld.pkt)) {
	case 1:
		if (ld.u) val = *(uint8_t *)data;
		else val = *(int8_t *)data;
		break;

	case 2: if (ld.u) val = *(uint16_t *)data;
		else val = *(int16_t *)data;
		break;

	case 4: if (ld.u) val = *(uint32_t *)data;
		else val = *(int32_t *)data;
		break;

	case 8: val = *(uint64_t *)data;
		break;

	default:
		error("unknown load size %zu", packet_size(ld.pkt));
	}

	set_reg(cpu, ld.reg, val);
}

static void finalize_st(struct simple_riscv64 *cpu)
{
	(void)cpu;
	/* nothing really to do, this is here mostly for vibe */
}

static void finalize_dls(struct simple_riscv64 *cpu)
{
	struct packet *pkt = cpu->dls.pkt;

	if (packet_type(pkt) == PACKET_READ)
		finalize_ld(cpu);
	else if (packet_type(pkt) == PACKET_WRITE)
		finalize_st(cpu);
	else
		error("unsupported packet type for simple_riscv64");

	destroy_packet(pkt);
	cpu->dls.pkt = NULL;
}

static uint32_t finalize_ils(struct simple_riscv64 *cpu)
{
	uint32_t insn = *(uint32_t *)packet_data(cpu->ils.pkt);
	destroy_packet(cpu->ils.pkt);
	cpu->ils.pkt = NULL;
	return insn;
}

static stat simple_riscv64_clock(struct simple_riscv64 *cpu)
{
	/* there's an active data transfer we should handle */
	if (cpu->dls.pkt) {
		assert(packet_state(cpu->dls.pkt) != PACKET_FAILED);

		if (packet_state(cpu->dls.pkt) == PACKET_DONE)
			finalize_dls(cpu);
		else /* wait for data */
			return OK;
	}

	if (!cpu->ils.pkt) {
		cpu->ils.pkt =
			create_packet(PACKET_READ, cpu->pc, sizeof(uint32_t));
		if (!cpu->ils.pkt)
			return EMEM;

		stat ret = read(cpu->imem, cpu->ils.pkt);
		if (ret)
			return ret;
	}

	uint32_t insn = 0;
	if (cpu->ils.pkt) {
		assert(packet_state(cpu->ils.pkt) != PACKET_FAILED);

		if (packet_state(cpu->ils.pkt) == PACKET_DONE)
			insn = finalize_ils(cpu);
		else /* wait for instruction */
			return OK;
	}

	// for now assume little endian emulated and host cpu
	union rv_insn i = {.val = insn};

	stat ret = OK;
	// all formats have identical opcodes, use whatever
	switch (i.rtype.op) {
	case OP_IMM: ret = op_imm(cpu, i); break;
	case OP_IMM_32: ret = op_imm_32(cpu, i); break;
	case LUI: ret = lui(cpu, i); break;
	case AUIPC: ret = auipc(cpu, i); break;
	case OP: ret = op(cpu, i); break;
	case JAL: ret = jal(cpu, i); break;
	case JALR: ret = jalr(cpu, i); break;
	case BRANCH: ret = branch(cpu, i); break;
	case LOAD: ret = load(cpu, i); break;
	case STORE: ret = store(cpu, i); break;
	case MISC_MEM: /* nop in this case */ break;
	case SYSTEM:
		/* we don't support these yet, but we can use them
		 * to stop the simulation */
		return DONE;
	default:
		error("unknown opcode %x", i.rtype.op);
		return ENOSUCH;
	}

	return ret;
}

static void simple_riscv64_destroy(struct simple_riscv64 *cpu)
{
	/* oh yeah, will have to think about the name stuff,
	 * i.e. how and where to free it, and where to assign it */
	destroy(cpu->imem);

	if (cpu->imem != cpu->dmem)
		destroy(cpu->dmem);

	if (cpu->ils.pkt)
		destroy_packet(cpu->ils.pkt);

	if (cpu->dls.pkt)
		destroy_packet(cpu->dls.pkt);

	free(cpu);
}

struct component *create_simple_riscv64(uint32_t start_pc,
                                        struct component *imem,
                                        struct component *dmem)
{
	struct simple_riscv64 *new = calloc(1, sizeof(struct simple_riscv64));
	if (!new)
		return NULL;

	new->component.clock = (clock_callback)simple_riscv64_clock;
	new->component.destroy = (destroy_callback)simple_riscv64_destroy;

	new->pc = start_pc;
	new->imem = imem;
	new->dmem = dmem;
	return (struct component *)new;
}

void simple_riscv64_set_reg(struct component *cpu, size_t reg, uint64_t val)
{
	struct simple_riscv64 *rv64 = (struct simple_riscv64 *)cpu;
	set_reg(rv64, reg, val);
}
