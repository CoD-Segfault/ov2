// Copyright 2011-2013 Segher Boessenkool <segher@kernel.crashing.org>
// Licensed to you under the terms of the GPL version 3; see file COPYING.

#include <xs1.h>

#include "vita.h"

#include "service.h"
#include "fpga-link.h"


int printgrr(const char fmt[], ...);
#define printf printgrr


static u32 link_chan;

static void fpga_write_reg(u32 reg, u32 x)
{
	out32(link_chan, reg);
	out32(link_chan, x);
}

static u32 fpga_read_reg(u32 reg)
{
	out32(link_chan, reg + 0x80000000);
	return in32(link_chan);
}

void fpga_reg_main(u32 c)
{
	for (;;) {
		u32 reg = in32(c);
		if (reg < 0x80000000) {
			fpga_write_reg(reg, in32(c));
			inend(c);
		} else {
			setd(c, in32(c));
			inend(c);
			out32(c, fpga_read_reg(reg - 0x80000000));
			outend(c);
		}
	}
}

void fpga_write32(u32 reg, u32 x)
{
	u32 c = new_chanend();

	setd(c, services[SERVICE_FPGA_REG]);
	out32(c, reg);
	out32(c, x);
	outend(c);

	free_chanend(c);
}

u32 fpga_read32(u32 reg)
{
	u32 c = new_chanend();

	setd(c, services[SERVICE_FPGA_REG]);
	out32(c, reg + 0x80000000);
	out32(c, c);
	outend(c);
	u32 x = in32(c);
	inend(c);

	free_chanend(c);

	return x;
}


static void init_2w_link_half(u32 n, u32 bittime)
{
	u32 id = get_core_id();
	bittime--;

	for (;;) {
		u32 x;

		write_sswitch_reg(id, 0x80 + n, 0x81800000 + 0x1001*bittime);
		udelay(1000);
		read_sswitch_reg(id, 0x80 + n, &x);
		if (x & 0x02000000)
			break;
	}
}

static void init_fpga_link_half(u32 n, u32 bittime)
{
	u32 id = get_core_id();
	bittime--;

	for (;;) {
		u32 x;

		fpga_write_reg(0x7f000080, 0x81800000 + 0x1001*bittime);
		udelay(1000);
		read_sswitch_reg(id, 0x80 + n, &x);
		if ((x & 0x06000000) == 0x06000000)
			break;
	}
}

void fpga_link_init(void)
{
	u32 id = get_core_id();

	link_chan = new_chanend();

	// Set up direction for the FPGA link.
	write_sswitch_reg(id, 0x0d, 0x01000000);
	write_sswitch_reg(id, 0x20+2, 1 << 8);

	// Set up static forwarding.
	write_sswitch_reg(id, 0xa0+2, 0x80000000 + ((link_chan >> 8) & 0xff));

	// Init link Xcore -> FPGA.
	init_2w_link_half(2, 16);	// 400MHz/16 = 25MHz
	printf("Link up one way.\n");

	// Set target for our chanend.
	setd(link_chan, 0xc0000002);

	// Init link FPGA -> Xcore.
	init_fpga_link_half(2, 2);	// 50MHz/2 = 25MHz
	printf("Link up both ways.\n");

	printf("(FPGA link reg = %08x)\n", fpga_read_reg(0x7f000080));
}
