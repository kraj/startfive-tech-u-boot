// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2022 Starfive, Inc.
 * Author:	yanhong <yanhong.wang@starfivetech.com>
 *
 */

#include "starfive_ddr.h"
#include <linux/delay.h>

#define DDRSEC_SEL	0x80000000
#define REGOFFSET(offset)	((offset) / 4)

static struct ddr_reg_set ddr_cfg[] = {
	{0x0,			0x00000001},
	{(DDRSEC_SEL | 0xf00),	0x40001030},
	{(DDRSEC_SEL | 0xf04),	0x00000001},
	{(DDRSEC_SEL | 0xf10),	0x00400000},
	{(DDRSEC_SEL | 0xf14),	0x043fffff},
	{(DDRSEC_SEL | 0xf18),	0x00000000},
	{(DDRSEC_SEL | 0xf30),	0x1f000041},
	{(DDRSEC_SEL | 0xf34),	0x1f000041},
	{(DDRSEC_SEL | 0x110),	0xc0000001},
	{(DDRSEC_SEL | 0x114),	0xffffffff},
	{0x10c,			0x00000505},
	{0x11c,			0x00000000},
	{0x500,			0x00000201},
	{0x514,			0x00000100},
	{0x6a8,			0x00040000},
	{0xea8,			0x00040000},
	{0x504,			0x40000000}
};

static struct ddr_reg_set ddr_cfg1[] = {
	{0x310,		0x00020000},
	{0x310,		0x00020001},
	{0x600,		0x002e0176},
	{0x604,		0x002e0176},
	{0x608,		0x001700bb},
	{0x60c,		0x000b005d},
	{0x610,		0x0005002e},
	{0x614,		0x00020017},
	{0x618,		0x00020017},
	{0x61c,		0x00020017},
	{0x678,		0x00000019},
	{0x100,		0x000000f8},
	{0x620,		0x03030404},
	{0x624,		0x04030505},
	{0x628,		0x07030884},
	{0x62c,		0x13150401},
	{0x630,		0x17150604},
	{0x634,		0x00110000},
	{0x638,		0x200a0a08},
	{0x63c,		0x1730f803},
	{0x640,		0x000a0c00},
	{0x644,		0xa005000a},
	{0x648,		0x00000000},
	{0x64c,		0x00081306},
	{0x650,		0x04070304},
	{0x654,		0x00000404},
	{0x658,		0x00000060},
	{0x65c,		0x00030008},
	{0x660,		0x00000000},
	{0x680,		0x00000603},
	{0x684,		0x01000202},
	{0x688,		0x0413040d},
	{0x68c,		0x20002420},
	{0x690,		0x00140000},
	{0x69c,		0x01240074},
	{0x6a0,		0x00000000},
	{0x6a4,		0x20240c00},
	{0x6a8,		0x00040000},
	{0x4,		0x30010006},
	{0xc,		0x00000002},
	{0x4,		0x30020000},
	{0xc,		0x00000002},
	{0x4,		0x30030031},
	{0xc,		0x00000002},
	{0x4,		0x300b0033},
	{0xc,		0x00000002},
	{0x4,		0x30160016},
	{0xc,		0x00000002},
	{0x10,		0x00000010},
	{0x14,		0x00000001},
};

static struct ddr_reg_clrset ddr_cfg2[] = {
	{46,	~(0xF << 24),	(0x3 << 24)},
	{33,	~(0x1 << 24),	0x0},
	{44,	~(0x1 << 16),	0x0},
	{44,	~(0x1 << 24),	0x0},
	{45,	0xffffffff,	0x1},
	{146,	0xffffffff,	(0x3 << 24)},
	{147,	0xffffffff,	(0x3 << 8)},
	{147,	0xffffffff,	(0x3 << 24)},
	{44,	0xffffffff,	(0x1 << 8)},
	{46,	~(0xF << 16),	(0x3 << 16)},
	{33,	~(0x1 << 16),	0x0},
	{43,	~(0x1 << 16),	0x0},
	{43,	~(0x1 << 24),	0x0},
	{44,	0xffffffff,	0x1},
	{146,	0xffffffff,	(0x3 << 16)},
	{147,	0xffffffff,	0x3},
	{147,	0xffffffff,	(0x3 << 16)},
	{148,	0xffffffff,	(0x3 << 24)},
	{149,	0xffffffff,	(0x3 << 24)},
	{150,	0xffffffff,	(0x3 << 24)},
	{43,	0xffffffff,	(0x1 << 8)},
	{67,	~(0xF << 8),	(0x3 << 8)},
	{68,	~(0x1 << 8),	0x0},
	{71,	~(0x1 << 16),	0x0},
	{71,	~(0x1 << 24),	0x0},
	{72,	0xffffffff,	(0x1 << 8)},
	{180,	0xffffffff,	(0x3 << 8)},
	{183,	0xffffffff,	(0x3 << 8)},
	{186,	0xffffffff,	(0x3 << 8)},
};

static struct ddr_reg_set ddr_cfg3[] = {
	{0x100,	0x000000e0},
	{0x620,	0x04041417},
	{0x624,	0x09110609},
	{0x628,	0x442d0994},
	{0x62c,	0x271e102b},
	{0x630,	0x291b140a},
	{0x634,	0x001c0000},
	{0x638,	0x200f0f08},
	{0x63c,	0x29420a06},
	{0x640,	0x019e1fc1},
	{0x644,	0x10cb0196},
	{0x648,	0x00000000},
	{0x64c,	0x00082714},
	{0x650,	0x16442f0d},
	{0x654,	0x00001916},
	{0x658,	0x00000060},
	{0x65c,	0x00600020},
	{0x660,	0x00000000},
	{0x680,	0x0c00040f},
	{0x684,	0x03000604},
	{0x688,	0x0515040d},
	{0x68c,	0x20002c20},
	{0x690,	0x00140000},
	{0x69c,	0x01240074},
	{0x6a0,	0x00000000},
	{0x6a4,	0x202c0c00},
	{0x6a8,	0x00040000},
	{0x4,	0x30010036},
	{0xc,	0x00000002},
	{0x4,	0x3002001b},
	{0xc,	0x00000002},
	{0x4,	0x30030031},
	{0xc,	0x00000002},
	{0x4,	0x300b0066},
	{0xc,	0x00000002},
	{0x4,	0x30160016},
	{0xc,	0x00000002},
	{0x410,	0x00101010},
	{0x420,	0x0c181006},
	{0x424,	0x20200820},
	{0x428,	0x80000020},
	{0x0,	0x00000001},
	{0x108,	0x00003000},
	{DDRSEC_SEL | 0x704,	0x00000007},
	{0x330,	0x09313fff},
	{0x508,	0x00000033},
	{0x324,	0x00002000},
	{0x104,	0x90000000},
	{0x510,	0x00000100},
	{0x514,	0x00000000},
	{DDRSEC_SEL | 0x700,	0x00000003},
	{0x514,	0x00000600},
	{0x20,	0x00000001},
};

void ddrcsr_boot(u32 *csrreg, u32 *secreg, u32 *phyreg)
{
	u32 i, len;
	u32 *addr;
	u32 val;

	len = sizeof(ddr_cfg)/sizeof(struct ddr_reg_set);
	for (i = 0; i < len; i++) {
		if (ddr_cfg[i].offset & DDRSEC_SEL)
			addr = secreg + REGOFFSET(ddr_cfg[i].offset & (~DDRSEC_SEL));
		else
			addr = csrreg + REGOFFSET(ddr_cfg[i].offset);
		out_le32(addr, ddr_cfg[i].val);
	}

	do {
		udelay(1);
		val = in_le32(csrreg + REGOFFSET(0x504));
	} while ((val & 0x80000000) != 0x80000000);

	out_le32(csrreg + REGOFFSET(0x504), 0x00000000);
	out_le32(csrreg + REGOFFSET(0x50c), 0x00000000);
	udelay(300);
	out_le32(csrreg + REGOFFSET(0x50c), 0x00000001);
	udelay(3000);
	out_le32(csrreg + REGOFFSET(0x10), 0x0000003c);
	out_le32(csrreg + REGOFFSET(0x14), 0x00000001);
	udelay(4);

	len = sizeof(ddr_cfg1) / sizeof(struct ddr_reg_set);
	for (i = 0; i < len; i++)
		out_le32(csrreg + REGOFFSET(ddr_cfg1[i].offset), ddr_cfg1[i].val);

	udelay(4);
	out_le32(csrreg + REGOFFSET(0x10), 0x00000011);
	out_le32(csrreg + REGOFFSET(0x14), 0x00000001);
	out_le32(csrreg + REGOFFSET(0x10), 0x00000020);
	out_le32(csrreg + REGOFFSET(0x14), 0x00000001);

	udelay(4);
	out_le32(csrreg + REGOFFSET(0x10), 0x00000021);
	out_le32(csrreg + REGOFFSET(0x14), 0x00000001);
	out_le32(csrreg + REGOFFSET(0x514), 0x00000000);

	do {
		udelay(1);
		val = in_le32(csrreg + REGOFFSET(0x518));
	} while ((val & 0x2) != 0x2);

	val = in_le32(csrreg + REGOFFSET(0x518));
	while ((val & 0x2) != 0x0) {
		val = in_le32(phyreg + 1);
		if ((val & 0x20) == 0x20) {
			switch (val & 0x0000001f) {
			case 0: //ddrc_clock=12M
				clrsetbits_le32(CLK_DDR_BUS_REG,
					CLK_DDR_BUS_MASK, 0<<24);
				break;
			case 1: //ddrc_clock=200M
				clrsetbits_le32(CLK_DDR_BUS_REG,
					CLK_DDR_BUS_MASK, 3<<24);
				break;
			case 2: //ddrc_clock=800M
				clrsetbits_le32(CLK_DDR_BUS_REG,
					CLK_DDR_BUS_MASK, 1<<24);
				break;
			default:
				break;
			};

			out_le32((phyreg + 2), 0x1);
			do {
				udelay(2);
				val = in_le32(phyreg + 2);
			} while ((val & 0x00000001) != 0x00000000);
		}
		udelay(1);
		val = in_le32(csrreg + REGOFFSET(0x518));
	};

	val = in_le32(phyreg + 2048 + 83);
	val = in_le32(phyreg + 2048 + 84);
	out_le32((phyreg + 2048 + 84), val & 0xF8000000);

	len = sizeof(ddr_cfg2) / sizeof(struct ddr_reg_clrset);
	for (i = 0; i < len; i++) {
		addr = phyreg + 2048;
		addr += ddr_cfg2[i].offset;
		DDR_REG_TRIGGER(addr, ddr_cfg2[i].mask, ddr_cfg2[i].val);
	}

	len = sizeof(ddr_cfg3)/sizeof(struct ddr_reg_set);
	for (i = 0; i < len; i++) {
		if (ddr_cfg3[i].offset & DDRSEC_SEL)
			addr = secreg + REGOFFSET(ddr_cfg3[i].offset & (~DDRSEC_SEL));
		else
			addr = csrreg + REGOFFSET(ddr_cfg3[i].offset);

		out_le32(addr, ddr_cfg3[i].val);
	}
}
