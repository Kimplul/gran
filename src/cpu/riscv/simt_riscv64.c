#include <gran/cpu/riscv/simt_riscv64.h>

/* use simple rv64 cores are lanes for now at least, possibly extend simple
 * model with some cross-lane interface for barriers etc. */
#include <gran/cpu/riscv/simple_riscv64.h>

/* vector of riscv cores */
#define VEC_NAME lanes
#define VEC_TYPE struct component *
#include <conts/vec.h>

/* vector of requests */
struct req {
	struct reg reg;
	bool queued;
};

#define VEC_NAME reqs
#define VEC_TYPE struct req
#include <conts/vec.h>

struct simt_riscv64 {
	struct component component;
	struct simt_riscv64_conf conf;
	struct component *imem;
	struct component *dmem;

	struct lanes lanes;

	struct component imem_intf;
	struct component dmem_intf;

	struct reqs data_rqs;
	struct reqs inst_rqs;

	size_t rr;
};

static stat simt_riscv64_ext_send(uint64_t rcv, struct component *intf, struct component *mem, struct packet pkt)
{
	/* do fixups for sending */
	if (is_set(&pkt, PACKET_READ))
		pkt.mask = ~0ULL;

	/* smuggle index as receive address */
	uint32_t i = (uint32_t)(pkt.from >> 32);
	pkt.from = rcv | i;
	return SEND(intf, mem, pkt);
}

static stat simt_riscv64_handle_response(struct simt_riscv64 *c,
		struct component *intf, struct reqs *reqs,
		struct packet pkt)
{
	/* use index smuggled as address to check which request this is a
	 * response to */
	uint32_t idx = (uint32_t)pkt.to;
	struct req *s = reqs_at(reqs, (size_t)idx);

	/* sanity check response, a bit crude for now */
	assert(s);
	assert(s->reg.pkt.to == pkt.from);
	assert(s->reg.busy == true);
	assert( is_set(&pkt, PACKET_DONE));
	assert(!is_set(&pkt, PACKET_ERROR));

	/* reads are always full width to maximize chance of all lanes hitting
	 * same cacheline.
	 *
	 * Probably not ideal for instructions when not lockstepping...?
	 */
	assert(is_set(&pkt, PACKET_READ) ? pkt.mask == ~0ULL : 1);

	/* use mask to indicate which portion of packet was meant for
	 * this lane */
	pkt.mask = s->reg.pkt.mask;

	/* use original from address */
	pkt.to = s->reg.pkt.from;

	struct component *lane = *lanes_at(&c->lanes, idx);
	stat ok = SEND(intf, lane, pkt);
	assert(ok == OK);

	s->reg.busy = false;

	/* possibly broadcast response to readers */
	if (is_set(&pkt, PACKET_READ))
	for (size_t i = 0; i < c->conf.num_lanes; ++i) {
		struct req *req = reqs_at(reqs, i);
		if (!req->reg.busy)
			continue;

		/* 64-byte aligned */
		if (req->reg.pkt.to != pkt.from)
			continue;

		/* skip non-reads */
		if (!is_set(&req->reg.pkt, PACKET_READ))
			continue;

		struct component *lane = *lanes_at(&c->lanes, i);

		pkt.mask = req->reg.pkt.mask;
		pkt.to = req->reg.pkt.from;

		stat ok = SEND(intf, lane, pkt);
		/* no reason for core to be blocked */
		assert(ok == OK);

		req->reg.busy = false;
	}

	return OK;
}

static stat simt_riscv64_receive(struct simt_riscv64 *c,
		size_t rcv, struct component *mem,
		struct component *intf, struct reqs *reqs,
		struct component *from, struct packet pkt)
{
	if (mem == from)
		return simt_riscv64_handle_response(c, intf, reqs, pkt);

	/* handle send */
	int32_t idx = pkt.from >> 32;
	struct req *req = reqs_at(reqs, idx);
	if (req->reg.busy)
		return EBUSY;

	/* store packet */
	req->reg.pkt = pkt;
	req->reg.busy = true;
	req->queued = false;

	/* check if packet can piggyback off of another packet */
	if (is_set(&pkt, PACKET_READ))
	foreach(reqs, other_req, reqs) {
		/* skip ourselves */
		if (req == other_req)
			continue;

		if (!other_req->reg.busy)
			continue;

		struct packet *other_pkt = &other_req->reg.pkt;
		if (!is_set(other_pkt, PACKET_READ))
			continue;

		/* yes, we can piggyback, therefore no need to do anything else */
		if (pkt.to == other_pkt->to)
			return OK;
	}

	stat r = simt_riscv64_ext_send(rcv, intf, mem, pkt);
	if (r == EBUSY)
		req->queued = true;

	return r;
}

static stat simt_riscv64_data_receive(struct component *dmem_intf, struct component *from,
			 struct packet pkt)
{
	struct simt_riscv64 *c = CONTAINER_OF(dmem_intf, struct simt_riscv64, dmem_intf);
	return simt_riscv64_receive(c, c->conf.data_rcv,
			c->dmem, &c->dmem_intf, &c->data_rqs,
			from, pkt
	);
}

static stat simt_riscv64_inst_receive(struct component *imem_intf, struct component *from,
		struct packet pkt)
{
	struct simt_riscv64 *c = CONTAINER_OF(imem_intf, struct simt_riscv64, imem_intf);
	return simt_riscv64_receive(c, c->conf.inst_rcv,
			c->imem, &c->imem_intf, &c->inst_rqs,
			from, pkt
	);
}

static stat simt_riscv64_clock(struct simt_riscv64 *c)
{
	stat r = OK;
	/* send out queued stuff */
	foreach(reqs, req, &c->data_rqs) {
		if (!req->queued)
			continue;

		r = simt_riscv64_ext_send(c->conf.data_rcv, &c->dmem_intf, c->dmem, req->reg.pkt);
		if (r == OK) {
			req->queued = false;
			continue;
		}

		if (r == EBUSY)
			continue;

		return r;
	}

	foreach(reqs, req, &c->inst_rqs) {
		if (!req->queued)
			continue;

		r = simt_riscv64_ext_send(c->conf.inst_rcv, &c->imem_intf, c->imem, req->reg.pkt);
		if (r == OK) {
			req->queued = false;
			continue;
		}

		if (r == EBUSY)
			continue;

		return r;
	}


	foreach(lanes, core, &c->lanes) {
		assert(core);

		if ((r = (*core)->clock(*core)) != OK)
			return r;
	}

	return r;
}

struct component *create_simt_riscv64(
		struct simt_riscv64_conf conf,
		struct component *imem,
		struct component *dmem)
{
	struct simt_riscv64 *new = calloc(1, sizeof(struct simt_riscv64));
	if (!new)
		return NULL;

	new->component.clock = (clock_callback)simt_riscv64_clock;

	new->conf = conf;
	new->imem = imem;
	new->dmem = dmem;

	new->lanes = lanes_create(conf.num_lanes);
	new->data_rqs = reqs_create(conf.num_lanes);
	new->inst_rqs = reqs_create(conf.num_lanes);

	new->imem_intf.receive = (receive_callback)simt_riscv64_inst_receive;
	new->dmem_intf.receive = (receive_callback)simt_riscv64_data_receive;

	new->rr = 0;

	for (uint64_t i = 0; i < conf.num_lanes; ++i) {
		struct component *core = create_simple_riscv64(
				/* core ID, should group id also be given? */
				i << 32,
				conf.start_pc,
				&new->imem_intf,
				&new->dmem_intf

		);

		lanes_append(&new->lanes, core);

		struct req empty = {
			.reg = {
				.pkt = {},
				.busy = false,
			},
			.queued = false
		};
		reqs_append(&new->data_rqs, empty);
		reqs_append(&new->inst_rqs, empty);
	}

	return (struct component *)&new->component;
}

struct component *simt_riscv64_data_intf(struct component *c)
{
	struct simt_riscv64 *rv64 = (struct simt_riscv64 *)c;
	return &rv64->dmem_intf;
}

struct component *simt_riscv64_inst_intf(struct component *c)
{
	struct simt_riscv64 *rv64 = (struct simt_riscv64 *)c;
	return &rv64->imem_intf;
}

void simt_riscv64_set_reg(struct component *c, size_t lane, size_t reg, uint64_t val)
{
	struct simt_riscv64 *rv64 = (struct simt_riscv64 *)c;
	simple_riscv64_set_reg(*lanes_at(&rv64->lanes, lane), reg, val);
}
