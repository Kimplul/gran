/* SPDX-License-Identifier: copyleft-next-0.3.1 */
/* Copyright 2023 Kim Kuparinen < kimi.h.kuparinen@gmail.com > */

#include <byteswap.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include <gran/cpu/riscv/simple_riscv64.h>

enum ldst_state {
	LDST_IDLE, LDST_BLOCKED, LDST_SENT, LDST_DONE
};

struct ldst {
	struct packet pkt;
	enum ldst_state state;
	uint32_t reg;
	bool u;
};

struct simple_riscv64 {
	struct component component;

	struct component *imem;
	struct component *dmem;
	struct component *ctrl;

	struct ldst dls;
	struct ldst ils;
	struct ldst cls;

	uint64_t rcv;

	/* have to be careful with x0 */
	uint64_t regs[32];
	uint64_t pc;

	bool sleep;
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
#define SHAMT(x) ((x) & 0b111111)

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
	/* LW/LWU */
	case 0b110: u = true; /* fallthrough */
	case 0b010: size = 4; break;
	/* LD */
	case 0b011: size = 8; break;
	default:
		error("unknown LOAD width %x", insn.btype.funct3);
		return ENOSUCH;
	}

	struct packet pkt = create_packet(cpu->rcv,
	                                  addr,
	                                  size,
	                                  NULL,
	                                  PACKET_READ);

	cpu->dls = (struct ldst){pkt, LDST_SENT, insn.itype.rd, u};
	stat ret = SEND(cpu, cpu->dmem, pkt);
	if (ret == EBUSY) {
		cpu->dls.state = LDST_BLOCKED;
		ret = OK;
	}

	cpu->pc += 4;
	return ret;
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
	uint64_t size = 0;

	switch (insn.stype.funct3) {
	/* SB */
	case 0b000: size = 1; break;
	/* SH */
	case 0b001: size = 2; break;
	/* SW */
	case 0b010: size = 4; break;
	/* SD */
	case 0b011: size = 8; break;
	default:
		error("unknown width of STORE %x", insn.stype.funct3);
		return ENOSUCH;
	}

	struct packet pkt = create_packet(cpu->rcv, addr, size, &src,
	                                  PACKET_WRITE);
	cpu->dls = (struct ldst){pkt, LDST_SENT, 0, false};
	stat ret = SEND(cpu, cpu->dmem, pkt);
	if (ret == EBUSY) {
		cpu->dls.state = LDST_BLOCKED;
		ret = OK;
	}

	cpu->pc += 4;
	return ret;
}

static void finalize_ld(struct simple_riscv64 *cpu)
{
	struct ldst ld = cpu->dls;

	uint64_t val = 0;
	switch (packet_convsize(&ld.pkt)) {
	case 1:
		if (ld.u) val = packet_convu8(&ld.pkt);
		else val = packet_convi8(&ld.pkt);
		break;

	case 2: if (ld.u) val = packet_convu16(&ld.pkt);
		else val = packet_convi16(&ld.pkt);
		break;

	case 4: if (ld.u) val = packet_convi32(&ld.pkt);
		else val = packet_convu32(&ld.pkt);
		break;

	case 8: val = packet_convu64(&ld.pkt);
		break;

	default:
		error("unknown load size %zu", packet_convsize(&ld.pkt));
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
	cpu->dls.state = LDST_IDLE;
	struct packet pkt = cpu->dls.pkt;

	if (is_set(&pkt, PACKET_READ))
		finalize_ld(cpu);
	else if (is_set(&pkt, PACKET_WRITE))
		finalize_st(cpu);
	else
		error("unsupported packet type for simple_riscv64");
}

static uint32_t finalize_ils(struct simple_riscv64 *cpu)
{
	cpu->ils.state = LDST_IDLE;
	return packet_convu32(&cpu->ils.pkt);
}

static stat simple_riscv64_receive(struct simple_riscv64 *cpu,
                                   struct component *from, struct packet pkt)
{
	(void)from;

	if (pkt.to == cpu->rcv) {
		cpu->dls.pkt = pkt;
		cpu->dls.state = LDST_DONE;
		return OK;
	}
	else if (pkt.to == cpu->rcv + 64) {
		cpu->ils.pkt = pkt;
		cpu->ils.state = LDST_DONE;
		return OK;
	}
	else if (pkt.to == cpu->rcv + 128) {
		assert(packet_convsize(&pkt) == 8);
		if (cpu->cls.state == LDST_BLOCKED)
			return EBUSY;

		uint64_t v = packet_convu64(&pkt);
		if (v == 0) {
			/* sleep */
			cpu->sleep = true;
		}
		else if (v == 1) {
			/* wake */
			cpu->sleep = false;
		}
		else {
			error("illegal control %s\n", cpu->component.name);
			return EBUS;
		}

		pkt = response(pkt);

		cpu->ctrl = from;
		cpu->cls = (struct ldst){pkt, LDST_IDLE, 0, false};
		stat ret = SEND(cpu, cpu->ctrl, pkt);
		if (ret == EBUSY)
			cpu->cls.state = LDST_BLOCKED;

		return OK;
	}
	else {
		error("illegal receive on %s", cpu->component.name);
		return EBUS;
	}

	return OK;
}

static stat simple_riscv64_clock(struct simple_riscv64 *cpu)
{
	if (cpu->cls.state != LDST_IDLE) {
		assert(cpu->cls.state == LDST_BLOCKED);
		stat r = SEND(cpu, cpu->ctrl, cpu->cls.pkt);
		if (r == EBUSY)
			return OK;

		cpu->cls.state = LDST_IDLE;
	}

	if (cpu->sleep)
		return OK;

	stat ret = OK;

	/* there's an active data transfer we should handle */
	if (cpu->dls.state != LDST_IDLE) {
		assert(!is_set(&cpu->dls.pkt, PACKET_ERROR));

		if (cpu->dls.state == LDST_BLOCKED) {
			stat r = SEND(cpu, cpu->dmem, cpu->dls.pkt);
			if (r == EBUSY)
				return OK;

			cpu->dls.state = LDST_SENT;
			return OK;
		}

		if (cpu->dls.state == LDST_SENT)
			return OK;

		if (cpu->dls.state == LDST_DONE) {
			finalize_dls(cpu);
			cpu->dls.state = LDST_IDLE;
		}
		else
			return OK;
	}

	if (cpu->ils.state == LDST_BLOCKED)
		goto send;

	if (cpu->ils.state == LDST_SENT)
		return OK;

	/* this effectively encodes a NOP */
	uint32_t insn = OP_IMM;

	if (cpu->ils.state == LDST_DONE) {
		assert(!is_set(&cpu->dls.pkt, PACKET_ERROR));

		insn = finalize_ils(cpu);
		cpu->ils.state = LDST_IDLE;
	}

	// for now assume little endian emulated and host cpu
	union rv_insn i = {.val = insn};

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

	if (ret != OK)
		return ret;

	if (cpu->ils.state == LDST_IDLE) {
		cpu->ils.pkt = create_packet(cpu->rcv + 64,
		                             cpu->pc,
		                             sizeof(uint32_t),
		                             NULL,
		                             PACKET_READ);
		cpu->ils.state = LDST_BLOCKED;
	}

send:
	if (cpu->ils.state == LDST_BLOCKED) {
		stat ret = SEND(cpu, cpu->imem, cpu->ils.pkt);
		if (ret == EBUSY) {
			cpu->ils.state = LDST_BLOCKED;
			return OK;
		}

		if (ret != OK)
			return ret;

		cpu->ils.state = LDST_SENT;
	}

	return OK;
}

struct component *create_simple_riscv64(uint64_t rcv, uint64_t start_pc,
                                        struct component *imem,
                                        struct component *dmem)
{
	struct simple_riscv64 *new = calloc(1, sizeof(struct simple_riscv64));
	if (!new)
		return NULL;

	new->component.receive = (receive_callback)simple_riscv64_receive;
	new->component.clock = (clock_callback)simple_riscv64_clock;

	new->pc = start_pc;
	new->rcv = rcv;
	new->imem = imem;
	new->dmem = dmem;

	/* fetch new instruction at start */
	new->ils.state = LDST_BLOCKED;
	new->ils.pkt = create_packet(new->rcv + 64,
	                             new->pc,
	                             sizeof(uint32_t),
	                             NULL,
	                             PACKET_READ);

	return (struct component *)new;
}

void simple_riscv64_set_reg(struct component *cpu, size_t reg, uint64_t val)
{
	struct simple_riscv64 *rv64 = (struct simple_riscv64 *)cpu;
	set_reg(rv64, reg, val);
}
