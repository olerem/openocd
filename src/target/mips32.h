/***************************************************************************
 *   Copyright (C) 2008 by Spencer Oliver                                  *
 *   spen@spen-soft.co.uk                                                  *
 *                                                                         *
 *   Copyright (C) 2008 by David T.L. Wong                                 *
 *                                                                         *
 *   Copyright (C) 2011 by Drasko DRASKOVIC                                *
 *   drasko.draskovic@gmail.com                                            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.           *
 ***************************************************************************/

#ifndef MIPS32_H
#define MIPS32_H

#include "target.h"
#include "mips32_pracc.h"

#define MIPS32_COMMON_MAGIC		0xB320B320

/**
 * Memory segments (32bit kernel mode addresses)
 * These are the traditional names used in the 32-bit universe.
 */
#define KUSEG			0x00000000
#define KSEG0			0x80000000
#define KSEG1			0xa0000000
#define KSEG2			0xc0000000
#define KSEG3			0xe0000000

/** Returns the kernel segment base of a given address */
#define KSEGX(a)		((a) & 0xe0000000)

/** CP0 CONFIG regites fields */
#define MIPS32_CONFIG0_KU_SHIFT 25
#define MIPS32_CONFIG0_KU_MASK (0x7 << MIPS32_CONFIG0_KU_SHIFT)

#define MIPS32_CONFIG0_K0_SHIFT 0
#define MIPS32_CONFIG0_K0_MASK (0x7 << MIPS32_CONFIG0_K0_SHIFT)

#define MIPS32_CONFIG0_K23_SHIFT 28
#define MIPS32_CONFIG0_K23_MASK (0x7 << MIPS32_CONFIG0_K23_SHIFT)

#define MIPS32_CONFIG0_AR_SHIFT 10
#define MIPS32_CONFIG0_AR_MASK (0x7 << MIPS32_CONFIG0_AR_SHIFT)

#define MIPS32_CONFIG1_DL_SHIFT 10
#define MIPS32_CONFIG1_DL_MASK (0x7 << MIPS32_CONFIG1_DL_SHIFT)

#define MIPS32_ARCH_REL1 0x0
#define MIPS32_ARCH_REL2 0x1

#define MIPS32_SCAN_DELAY_LEGACY_MODE 2000000

/* offsets into mips32 core register cache */
enum {
	MIPS32_R2 = 2,
	MIPS32_R4 = 4,
	MIPS32_R5 = 5,
	MIPS32_R25 = 25,
	MIPS32_PC = 37,
	MIPS32_FIR = 71,
	MIPS32NUMCOREREGS
};

#define CACHE_REG_STATUS 32
#define CACHE_REG_CAUSE 36
#define CACHE_REG_PC 37

#define MIPS32NUMDSPREGS 9

#define MIPS32NUMCP0REGS 104

/* Bit Mask indicating CP0 register supported by this core */
#define	MIPS_CP0_MK4		0x0001
#define	MIPS_CP0_mAPTIV_uC	0x0002
#define	MIPS_CP0_mAPTIV_uP	0x0004
#define MIPS_CP0_iAPTIV		0x0008

static const struct {
	unsigned reg;
	unsigned sel;
	const char *name;
	const unsigned core;
} mips32_cp0_regs[MIPS32NUMCP0REGS] = {
	{0, 0, "index", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uP},
	{0, 1, "mvpcontrol", MIPS_CP0_iAPTIV},
	{0, 2, "mvpconf0", MIPS_CP0_iAPTIV},
	{0, 3, "mvpconf1", MIPS_CP0_iAPTIV},
	{1, 0, "random", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uP},
	{1, 1, "vpecontrol", MIPS_CP0_iAPTIV},
	{1, 2, "vpeconf0", MIPS_CP0_iAPTIV},
	{1, 3, "vpeconf1", MIPS_CP0_iAPTIV},
	{1, 4, "yqmask", MIPS_CP0_iAPTIV},
	{1, 5, "vpeschedule", MIPS_CP0_iAPTIV},
	{1, 6, "vpeschefback", MIPS_CP0_iAPTIV},
	{1, 7, "vpeopt", MIPS_CP0_iAPTIV},
	{2, 0, "entrylo0", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uP},
	{2, 1, "tcstatus", MIPS_CP0_iAPTIV},
	{2, 2, "tcbind", MIPS_CP0_iAPTIV},
	{2, 3, "tcrestart", MIPS_CP0_iAPTIV},
	{2, 4, "tchalt", MIPS_CP0_iAPTIV},
	{2, 5, "tccontext", MIPS_CP0_iAPTIV},
	{2, 6, "tcschedule", MIPS_CP0_iAPTIV},
	{2, 7, "tcschefback", MIPS_CP0_iAPTIV},
	{3, 0, "entrylo1",MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uP},
	{3, 7, "tcopt", MIPS_CP0_iAPTIV},
	{4, 0, "context", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uP},
	{4, 2, "userlocal", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uC | MIPS_CP0_mAPTIV_uP | MIPS_CP0_MK4},
	{5, 0, "pagemask", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uP},
	{5, 1, "pagegrain", MIPS_CP0_mAPTIV_uP},
	{5, 2, "segctl0", MIPS_CP0_iAPTIV},
	{5, 3, "segctl1", MIPS_CP0_iAPTIV},
	{5, 4, "segctl2", MIPS_CP0_iAPTIV},
	{6, 0, "wired", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uP},
	{6, 1, "srsconf0", MIPS_CP0_iAPTIV},
	{6, 2, "srsconf1", MIPS_CP0_iAPTIV},
	{6, 3, "srsconf2", MIPS_CP0_iAPTIV},
	{6, 4, "srsconf3", MIPS_CP0_iAPTIV},
	{6, 5, "srsconf4", MIPS_CP0_iAPTIV},
	{7, 0, "hwrena", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uC | MIPS_CP0_mAPTIV_uP | MIPS_CP0_MK4},
	{8, 0, "badvaddr", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uC | MIPS_CP0_mAPTIV_uP | MIPS_CP0_MK4},
	{8, 1, "badinstr", MIPS_CP0_mAPTIV_uC | MIPS_CP0_mAPTIV_uP},
	{8, 2, "badinstrp", MIPS_CP0_mAPTIV_uC | MIPS_CP0_mAPTIV_uP},
	{9, 0, "count", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uC | MIPS_CP0_mAPTIV_uP | MIPS_CP0_MK4},
	{10, 0, "entryhi", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uP},
	{11, 0, "compare", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uC | MIPS_CP0_mAPTIV_uP | MIPS_CP0_MK4},
	{12, 0, "status", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uC | MIPS_CP0_mAPTIV_uP | MIPS_CP0_MK4},
	{12, 1, "intctl", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uC | MIPS_CP0_mAPTIV_uP | MIPS_CP0_MK4},
	{12, 2, "srsctl", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uC | MIPS_CP0_mAPTIV_uP | MIPS_CP0_MK4},
	{12, 3, "srsmap", MIPS_CP0_iAPTIV},
	{12, 3, "srsmap1", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uC | MIPS_CP0_mAPTIV_uP},
	{12, 4, "view_ipl", MIPS_CP0_mAPTIV_uC | MIPS_CP0_mAPTIV_uP | MIPS_CP0_MK4},
	{12, 5, "srsmap2", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uC | MIPS_CP0_mAPTIV_uP},
	{13, 0, "cause", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uC | MIPS_CP0_mAPTIV_uP | MIPS_CP0_MK4},
	{13, 5, "nestedexc", MIPS_CP0_mAPTIV_uC | MIPS_CP0_mAPTIV_uP| MIPS_CP0_MK4},
	{14, 0, "epc", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uC | MIPS_CP0_mAPTIV_uP | MIPS_CP0_MK4},
	{14, 2, "nestedepc", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uC | MIPS_CP0_mAPTIV_uP | MIPS_CP0_MK4},
	{15, 0, "prid", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uC | MIPS_CP0_mAPTIV_uP | MIPS_CP0_MK4},
	{15, 1, "ebase", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uC | MIPS_CP0_mAPTIV_uP | MIPS_CP0_MK4},
	{15, 2, "cdmmbase", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uC | MIPS_CP0_mAPTIV_uP | MIPS_CP0_MK4},
	{15, 3, "cmgcrbase", MIPS_CP0_iAPTIV},
	{16, 0, "config", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uC | MIPS_CP0_mAPTIV_uP | MIPS_CP0_MK4},
	{16, 1, "config1", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uC | MIPS_CP0_mAPTIV_uP | MIPS_CP0_MK4},
	{16, 2, "config2", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uC | MIPS_CP0_mAPTIV_uP | MIPS_CP0_MK4},
	{16, 3, "config3", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uC | MIPS_CP0_mAPTIV_uP | MIPS_CP0_MK4},
	{16, 4, "config4", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uC | MIPS_CP0_mAPTIV_uP | MIPS_CP0_MK4},
	{16, 5, "config5", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uC | MIPS_CP0_mAPTIV_uP | MIPS_CP0_MK4},
	{16, 7, "config7", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uC | MIPS_CP0_mAPTIV_uP | MIPS_CP0_MK4},
	{17, 0, "lladdr", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uP | MIPS_CP0_MK4},
	{18, 0, "watchlo0", MIPS_CP0_iAPTIV},
	{18, 1, "watchlo1", MIPS_CP0_iAPTIV},
	{18, 2, "watchlo2", MIPS_CP0_iAPTIV},
	{18, 3, "watchlo3", MIPS_CP0_iAPTIV},
	{19, 0, "watchhi0", MIPS_CP0_iAPTIV},
	{19, 1, "watchhi1", MIPS_CP0_iAPTIV},
	{19, 2, "watchhi2", MIPS_CP0_iAPTIV},
	{19, 3, "watchhi3", MIPS_CP0_iAPTIV},
	{23, 0, "debug", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uC | MIPS_CP0_mAPTIV_uP | MIPS_CP0_MK4},
	{23, 1, "tracecontrol", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uC | MIPS_CP0_mAPTIV_uP | MIPS_CP0_MK4},
	{23, 2, "tracecontrol2", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uC | MIPS_CP0_mAPTIV_uP | MIPS_CP0_MK4},
	{23, 3, "usertracedata1", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uC | MIPS_CP0_mAPTIV_uP | MIPS_CP0_MK4},
	{23, 4, "tracebpc", MIPS_CP0_mAPTIV_uC | MIPS_CP0_mAPTIV_uP | MIPS_CP0_MK4},
	{23, 4, "traceibpc", MIPS_CP0_iAPTIV},
	{23, 5, "tracedbpc", MIPS_CP0_iAPTIV},
	{24, 0, "depc", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uC | MIPS_CP0_mAPTIV_uP | MIPS_CP0_MK4},
	{24, 2, "tracecontrol3", MIPS_CP0_iAPTIV},
	{24, 3, "usertracedata2", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uC | MIPS_CP0_mAPTIV_uP | MIPS_CP0_MK4},
	{25, 0, "perfctl0", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uC | MIPS_CP0_mAPTIV_uP | MIPS_CP0_MK4},
	{25, 1, "perfcnt0", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uC | MIPS_CP0_mAPTIV_uP | MIPS_CP0_MK4},
	{25, 2, "perfctl1", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uC | MIPS_CP0_mAPTIV_uP | MIPS_CP0_MK4},
	{25, 3, "perfcnt1", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uC | MIPS_CP0_mAPTIV_uP | MIPS_CP0_MK4},
	{26, 0, "errctl", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uC | MIPS_CP0_mAPTIV_uP | MIPS_CP0_MK4},
	{27, 0, "cacheerr", MIPS_CP0_iAPTIV},
	{28, 0, "itaglo", MIPS_CP0_iAPTIV},
	{28, 0, "taglo", MIPS_CP0_iAPTIV},
	{28, 1, "idatalo", MIPS_CP0_iAPTIV},
	{28, 1, "datalo", MIPS_CP0_iAPTIV},
	{28, 2, "dtaglo", MIPS_CP0_iAPTIV},
	{28, 3, "ddatalo", MIPS_CP0_iAPTIV},
	{28, 4, "l23taglo", MIPS_CP0_iAPTIV},
	{28, 5, "l23datalo", MIPS_CP0_iAPTIV},
	{29, 1, "idatahi", MIPS_CP0_iAPTIV},
	{29, 2, "dtaghi", MIPS_CP0_iAPTIV},
	{29, 5, "l23datahi", MIPS_CP0_iAPTIV},
	{30, 0, "errorepc", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uC | MIPS_CP0_mAPTIV_uP | MIPS_CP0_MK4},
	{31, 0, "desave", MIPS_CP0_iAPTIV | MIPS_CP0_mAPTIV_uC | MIPS_CP0_mAPTIV_uP | MIPS_CP0_MK4},
	{31, 2, "kscratch1", MIPS_CP0_mAPTIV_uC | MIPS_CP0_mAPTIV_uP},
	{31, 3, "kscratch2", MIPS_CP0_mAPTIV_uC | MIPS_CP0_mAPTIV_uP},
};
enum mips32_isa_mode {
	MIPS32_ISA_MIPS32 = 0,
	MIPS32_ISA_MIPS16E = 1,
};

enum micro_mips_enabled {
	MIPS32_ONLY = 0,
	MICRO_MIPS_ONLY = 1,
	MICRO_MIPS32_16_ONRESET_MIPS32 = 2,
	MICRO_MIPS32_16_ONRESET_MIPS16 = 3,
};

enum fp {
	FP_NOT_IMP = 0,
	FP_IMP = 1,
};

enum dsp {
	DSP_NOT_IMP = 0,
	DSP_IMP = 1,
};

enum dsp_rev {
	DSP_REV1 = 0,
	DSP_REV2 = 1,
};

enum semihosting {
	DISABLE_SEMIHOSTING = false,
	ENABLE_SEMIHOSTING = true,
};
struct mips32_comparator {
	int used;
	uint32_t bp_value;
	uint32_t reg_address;
};

struct mips32_common {
	uint32_t common_magic;
	void *arch_info;
	struct reg_cache *core_cache;
	struct mips_ejtag ejtag_info;
	uint32_t core_regs[MIPS32NUMCOREREGS];
	enum mips32_isa_mode isa_mode;
	enum micro_mips_enabled mmips;
	enum dsp dsp_implemented;
	enum dsp_rev dsp_rev;
	enum fp fp_implemented;
	int fdc;
	enum semihosting semihosting;

	/* Mask used to determine is CP0 name valid for core */
	/* note hard coded for M4K to support backward compatibility */
	uint32_t cp0_mask;

	/* working area for fastdata access */
	struct working_area *fast_data_area;

	int bp_scanned;
	int num_inst_bpoints;
	int num_data_bpoints;
	int num_inst_bpoints_avail;
	int num_data_bpoints_avail;
	struct mips32_comparator *inst_break_list;
	struct mips32_comparator *data_break_list;

	/* register cache to processor synchronization */
	int (*read_core_reg)(struct target *target, unsigned int num);
	int (*write_core_reg)(struct target *target, unsigned int num);
};

static inline struct mips32_common *
target_to_mips32(struct target *target)
{
	return target->arch_info;
}

struct mips32_core_reg {
	uint32_t num;
	struct target *target;
	struct mips32_common *mips32_common;
};

struct mips32_algorithm {
	int common_magic;
	enum mips32_isa_mode isa_mode;
};

#define zero	0

#define AT	1

#define v0	2
#define v1	3

#define a0	4
#define a1	5
#define a2	6
#define	a3	7
#define t0	8
#define t1	9
#define t2	10
#define t3	11
#define t4	12
#define t5	13
#define t6	14
#define t7	15
#define ta0	12	/* alias for $t4 */
#define ta1	13	/* alias for $t5 */
#define ta2	14	/* alias for $t6 */
#define ta3	15	/* alias for $t7 */

#define s0	16
#define s1	17
#define s2	18
#define s3	19
#define s4	20
#define s5	21
#define s6	22
#define s7	23
#define s8	30		/* == fp */

#define t8	24
#define t9	25
#define k0	26
#define k1	27

#define gp	28

#define sp	29
#define fp	30
#define ra	31

#define ALL 0
#define INST 1
#define DATA 2
#define ALLNOWB 3
#define DATANOWB 4
#define L2 5

/*
 * MIPS32 Config0 Register	(CP0 Register 16, Select 0)
 */
#define CFG0_M			0x80000000		/* Config1 implemented */
#define CFG0_BE			0x00008000		/* Big Endian */
#define CFG0_ATMASK		0x00006000		/* Architecture type: */
#define CFG0_AT_M32		(0<<13)			/* MIPS32 */
#define CFG0_AT_M64_A32 (1<<13)			/* MIPS64, 32-bit addresses */
#define CFG0_AT_M64_A64 (2<<13)			/* MIPS64, 64-bit addresses */
#define CFG0_AT_RES		(3<<13)
#define CFG0_ARMASK		0x00001c00
#define CFG0_ARSHIFT	10
#define CFG0_MTMASK		0x00000380
#define CFG0_MT_NONE	(0<<7)
#define CFG0_MT_TLB		(1<<7)
#define CFG0_MT_BAT		(2<<7)
#define CFG0_MT_NONSTD	(3<<7)
#define CFG0_VI			0x00000008		/* Icache is virtual */
#define CFG0_K0MASK		0x00000007		/* KSEG0 coherency algorithm */

/*
 * MIPS32 Config1 Register (CP0 Register 16, Select 1)
 */
#define CFG1_M			0x80000000		/* Config2 implemented */
#define CFG1_MMUSMASK	0x7e000000		/* mmu size - 1 */
#define CFG1_MMUSSHIFT	25
#define CFG1_ISMASK		0x01c00000		/* icache lines 64<<n */
#define CFG1_ISSHIFT	22
#define CFG1_ILMASK		0x00380000		/* icache line size 2<<n */
#define CFG1_ILSHIFT	19
#define CFG1_IAMASK		0x00070000		/* icache ways - 1 */
#define CFG1_IASHIFT	16
#define CFG1_DSMASK		0x0000e000		/* dcache lines 64<<n */
#define CFG1_DSSHIFT	13
#define CFG1_DLMASK		0x00001c00		/* dcache line size 2<<n */
#define CFG1_DLSHIFT	10

/*
 * MIPS32 Config1 Register (CP0 Register 16, Select 1)
 */
#define CFG1_M			0x80000000		/* Config2 implemented */
#define CFG1_MMUSMASK	0x7e000000		/* mmu size - 1 */
#define CFG1_MMUSSHIFT	25
#define CFG1_ISMASK		0x01c00000		/* icache lines 64<<n */
#define CFG1_ISSHIFT	22
#define CFG1_ILMASK		0x00380000		/* icache line size 2<<n */
#define CFG1_ILSHIFT	19
#define CFG1_IAMASK		0x00070000		/* icache ways - 1 */
#define CFG1_IASHIFT	16
#define CFG1_DSMASK		0x0000e000		/* dcache lines 64<<n */
#define CFG1_DSSHIFT	13
#define CFG1_DLMASK		0x00001c00		/* dcache line size 2<<n */
#define CFG1_DLSHIFT	10
#define CFG1_DAMASK		0x00000380		/* dcache ways - 1 */
#define CFG1_DASHIFT	7
#define CFG1_C2			0x00000040		/* Coprocessor 2 present */
#define CFG1_MD			0x00000020		/* MDMX implemented */
#define CFG1_PC			0x00000010		/* performance counters implemented */
#define CFG1_WR			0x00000008		/* watch registers implemented */
#define CFG1_CA			0x00000004		/* compression (mips16) implemented */
#define CFG1_EP			0x00000002		/* ejtag implemented */
#define CFG1_FP			0x00000001		/* fpu implemented */

/*
 * MIPS32r2 Config2 Register (CP0 Register 16, Select 2)
 */
#define CFG2_M			0x80000000		/* Config3 implemented */
#define CFG2_TUMASK		0x70000000		/* tertiary cache control */
#define CFG2_TUSHIFT	28
#define CFG2_TSMASK		0x0f000000		/* tcache sets per wway 64<<n */
#define CFG2_TSSHIFT	24
#define CFG2_TLMASK		0x00f00000		/* tcache line size 2<<n */
#define CFG2_TLSHIFT	20
#define CFG2_TAMASK		0x000f0000		/* tcache ways - 1 */
#define CFG2_TASHIFT	16
#define CFG2_SUMASK		0x0000f000		/* secondary cache control */
#define CFG2_SUSHIFT	12
#define CFG2_SSMASK		0x00000f00		/* scache sets per wway 64<<n */
#define CFG2_SSSHIFT	8
#define CFG2_SLMASK		0x000000f0		/* scache line size 2<<n */
#define CFG2_SLSHIFT	4
#define CFG2_SAMASK		0x0000000f		/* scache ways - 1 */
#define CFG2_SASHIFT	0

/*
 * MIPS32r2 Config3 Register (CP0 Register 16, Select 3)
 */
#define CFG3_M			0x80000000		/* Config4 implemented */
#define CFG3_ISAONEXC	0x00010000		/* ISA mode on exception entry */
#define CFG3_ISA_MODE	0x0000C000		/* ISA mode */
#define CFG3_DSP_REV	0x00000800		/* DSP Rev */
#define CFG3_DSPP		0x00000400		/* DSP ASE present */
#define CFG3_LPA		0x00000080		/* Large physical addresses */
#define CFG3_VEIC		0x00000040		/* Vectored external i/u controller */
#define CFG3_VI			0x00000020		/* Vectored i/us */
#define CFG3_SP			0x00000010		/* Small page support */
#define CFG3_MT			0x00000004		/* MT ASE present */
#define CFG3_SM			0x00000002		/* SmartMIPS ASE */
#define CFG3_TL			0x00000001		/* Trace Logic */

/* Enable CoProcessor - CP0 12, 0 (status) */
#define STATUS_CU3_MASK 0x80000000
#define STATUS_CU2_MASK 0x40000000
#define STATUS_CU1_MASK 0x20000000
#define STATUS_CU0_MASK 0x10000000

/*
 * Cache operations
 */
#define Index_Invalidate_I				 0x00		 /* 0		0 */
#define Index_Writeback_Inv_D			 0x01		 /* 0		1 */
#define Index_Writeback_Inv_T			 0x02		 /* 0		2 */
#define Index_Writeback_Inv_S			 0x03		 /* 0		3 */
#define Index_Load_Tag_I				 0x04		 /* 1		0 */
#define Index_Load_Tag_D				 0x05		 /* 1		1 */
#define Index_Load_Tag_T				 0x06		 /* 1		2 */
#define Index_Load_Tag_S				 0x07		 /* 1		3 */
#define Index_Store_Tag_I				 0x08		 /* 2		0 */
#define Index_Store_Tag_D				 0x09		 /* 2		1 */
#define Index_Store_Tag_T				 0x0A		 /* 2		2 */
#define Index_Store_Tag_S				 0x0B		 /* 2		3 */
#define Hit_Invalidate_I				 0x10		 /* 4		0 */
#define Hit_Invalidate_D				 0x11		 /* 4		1 */
#define Hit_Invalidate_T				 0x12		 /* 4		2 */
#define Hit_Invalidate_S				 0x13		 /* 4		3 */
#define Fill_I							 0x14		 /* 5		0 */
#define Hit_Writeback_Inv_D				 0x15		 /* 5		1 */
#define Hit_Writeback_Inv_T				 0x16		 /* 5		2 */
#define Hit_Writeback_Inv_S				 0x17		 /* 5		3 */
#define Hit_Writeback_D					 0x19		 /* 6		1 */
#define Hit_Writeback_T					 0x1A		 /* 6		1 */
#define Hit_Writeback_S					 0x1B		 /* 6		3 */
#define Fetch_Lock_I					 0x1C		 /* 7		0 */
#define Fetch_Lock_D					 0x1D		 /* 7		1 */

/*
 *    Cache Coherency Attribute
 *    
 *    0 Cacheable, noncoherent, write-through, no write allocate (microAptiv)
 *    0 Reserved (interAptiv)
 *
 *	  1 Cacheable, noncoherent, write-through, write allocate (microAptiv)
 *	  1 Reserved (interAptiv)
 *
 *	  2 Uncached (microAptiv, interAptiv)
 *
 *	  3 Cacheable, noncoherent, write-back, write allocate (microAptiv, interAptiv)
 *
 *	  4 Cacheable, coherent, write-back, write-allocate, read misses request Exclusive (interAptiv)
 *	  4 Cacheable, noncoherent, write-back, write allocate (interAptiv)
 *
 *	  5 Cacheable, noncoherent, write-back, write allocate (microAptiv)
 *	  5 Cacheable, coherent, write-back, write-allocate, read misses request Shared (interAptiv)
 *
 *	  6 Cacheable, noncoherent, write-back, write allocate (microAptiv)
 *	  6 Reserved (interAptiv)
 *
 *	  7 Uncached (microAptiv, interAptiv)
 */

#define CCA_WT_NOWA     0
#define CCA_WT_WA       1
#define CCA_UC          2
#define CCA_WB          3
#define CCA_IAPTIV_CWBE 4
#define CCA_IAPTIV_CWB  5
#define CCA_IAPTIV_RES  6
#define CCA_IAPTIV_UCA  7

/*
 * MIPS32 Coprocessor 0 register numbers
 */
#define C0_INDEX		0
#define C0_INX			0
#define C0_RANDOM		1
#define C0_RAND			1
#define C0_ENTRYLO0		2
#define C0_TLBLO0		2
#define C0_ENTRYLO1		3
#define C0_TLBLO1		3
#define C0_CONTEXT		4
#define C0_CTXT			4
#define C0_PAGEMASK		5
#define C0_PAGEGRAIN	(5, 1)
#define C0_WIRED		6
#define C0_HWRENA		7
#define C0_BADVADDR		8
#define C0_VADDR		8
#define C0_COUNT		9
#define C0_ENTRYHI		10
#define C0_TLBHI		10
#define C0_COMPARE		11
#define C0_STATUS		12
#define C0_SR			12
#define C0_INTCTL		(12, 1)
#define C0_SRSCTL		(12, 2)
#define C0_SRSMAP		(12, 3)
#define C0_CAUSE		13
#define C0_CR			13
#define C0_EPC			14
#define C0_PRID			15
#define C0_EBASE		(15, 1)
#define C0_CONFIG		16
#define C0_CONFIG0		(16, 0)
#define C0_CONFIG1		(16, 1)
#define C0_CONFIG2		(16, 2)
#define C0_CONFIG3		(16, 3)
#define C0_LLADDR		17
#define C0_WATCHLO		18
#define C0_WATCHHI		19
#define C0_DEBUG		23
#define C0_DEPC			24
#define C0_PERFCNT		25
#define C0_ERRCTL		26
#define C0_CACHEERR		27
#define C0_TAGLO		28
#define C0_ITAGLO		28
#define C0_DTAGLO		(28, 2)
#define C0_TAGLO2		(28, 4)
#define C0_DATALO		(28, 1)
#define C0_IDATALO		(28, 1)
#define C0_DDATALO		(28, 3)
#define C0_DATALO2		(28, 5)
#define C0_TAGHI		29
#define C0_ITAGHI		29
#define C0_DATAHI		(29, 1)
#define C0_ERRPC		30
#define C0_DESAVE		31

#define MIPS32_OP_ADD 0x20
#define MIPS32_OP_ADDU 0x21
#define MIPS32_OP_ADDIU 0x9
#define MIPS32_OP_ANDI	0x0C
#define MIPS32_OP_BEQ	0x04
#define MIPS32_OP_BGTZ	0x07
#define MIPS32_OP_BNE	0x05
#define MIPS32_OP_ADDI	0x08
#define MIPS32_OP_AND	0x24
#define MIPS32_OP_CACHE	0x2F
#define MIPS32_OP_COP0	0x10
#define MIPS32_OP_COP1	0x11
#define MIPS32_OP_EXT	0x1F
#define MIPS32_OP_J     0x02
#define MIPS32_OP_JR	0x08
#define MIPS32_OP_LUI	0x0F
#define MIPS32_OP_LW	0x23
#define MIPS32_OP_LBU	0x24
#define MIPS32_OP_LHU	0x25
#define MIPS32_OP_MFHI	0x10
#define MIPS32_OP_MTHI	0x11
#define MIPS32_OP_MFLO	0x12
#define MIPS32_OP_MUL	0x2
#define MIPS32_OP_MTLO	0x13
#define MIPS32_OP_RDHWR 0x3B
#define MIPS32_OP_SB	0x28
#define MIPS32_OP_SH	0x29
#define MIPS32_OP_SW	0x2B
#define MIPS32_OP_ORI	0x0D
#define MIPS32_OP_XORI	0x0E
#define MIPS32_OP_XOR	0x26
#define MIPS32_OP_TLB	0x10
#define MIPS32_OP_SLTU  0x2B
#define MIPS32_OP_SLLV	0x04
#define MIPS32_OP_SRA	0x03
#define MIPS32_OP_SRL	0x02
#define MIPS32_OP_SYNCI	0x1F

#define MIPS32_OP_REGIMM	0x01
#define MIPS32_OP_SDBBP		0x3F
#define MIPS32_OP_SPECIAL	0x00
#define MIPS32_OP_SPECIAL2	0x07
#define MIPS32_OP_SPECIAL3	0x1F

#define MIPS32_COP0_MF	0x00
#define MIPS32_COP1_MF	0x00
#define MIPS32_COP1_CF	0x02
#define MIPS32_COP0_MT	0x04
#define MIPS32_COP1_MT	0x04
#define MIPS32_COP1_CT	0x06

#define MIPS32_R_INST(opcode, rs, rt, rd, shamt, funct) \
	(((opcode) << 26) | ((rs) << 21) | ((rt) << 16) | ((rd) << 11) | ((shamt) << 6) | (funct))
#define MIPS32_I_INST(opcode, rs, rt, immd) \
	(((opcode) << 26) | ((rs) << 21) | ((rt) << 16) | (immd))

#define MIPS32_TLB_INST(opcode, co, rs, rt)						\
	(((opcode) << 26) | ((co) << 25) | ((rs) << 6) | (rt))


#define MIPS32_J_INST(opcode, addr)	(((opcode) << 26) | (addr))

#define MIPS32_NOP						0
#define MIPS32_ADD(dst, src, tar)		MIPS32_R_INST(MIPS32_OP_SPECIAL, src, tar, dst, 0, MIPS32_OP_ADD)
#define MIPS32_ADDI(tar, src, val)		MIPS32_I_INST(MIPS32_OP_ADDI, src, tar, val)
#define MIPS32_ADDIU(tar, src, val)		MIPS32_I_INST(MIPS32_OP_ADDIU, src, tar, val)
#define MIPS32_ADDU(dst, src, tar)		MIPS32_R_INST(MIPS32_OP_SPECIAL, src, tar, dst, 0, MIPS32_OP_ADDU)
#define MIPS32_AND(reg, off, val)		MIPS32_R_INST(MIPS32_OP_SPECIAL, off, val, reg, 0, MIPS32_OP_AND)
#define MIPS32_ANDI(tar, src, val)		MIPS32_I_INST(MIPS32_OP_ANDI, src, tar, val)
#define MIPS32_B(off)					MIPS32_BEQ(0, 0, off)
#define MIPS32_BEQ(src, tar, off)		MIPS32_I_INST(MIPS32_OP_BEQ, src, tar, off)
#define MIPS32_BGTZ(reg, off)			MIPS32_I_INST(MIPS32_OP_BGTZ, reg, 0, off)
#define MIPS32_BNE(src, tar, off)		MIPS32_I_INST(MIPS32_OP_BNE, src, tar, off)
#define MIPS32_CACHE(op, off, base)		MIPS32_I_INST(MIPS32_OP_CACHE, base, op, off)
#define MIPS32_EXT(dst, src, shf, sz)		MIPS32_R_INST(MIPS32_OP_EXT, src, dst, (sz-1), shf, 0)
#define MIPS32_J(tar)					MIPS32_J_INST(MIPS32_OP_J, tar)
#define MIPS32_JR(reg)					MIPS32_R_INST(0, reg, 0, 0, 0, MIPS32_OP_JR)
#define MIPS32_MFC0(gpr, cpr, sel)		MIPS32_R_INST(MIPS32_OP_COP0, MIPS32_COP0_MF, gpr, cpr, 0, sel)
#define MIPS32_MFC1(rt, fs)				MIPS32_R_INST(MIPS32_OP_COP1, MIPS32_COP1_MF, rt, fs, 0, 0)
#define MIPS32_CFC1(rt, fs)				MIPS32_R_INST(MIPS32_OP_COP1, MIPS32_COP1_CF, rt, fs, 0, 0)
#define MIPS32_MOVE(dst, src)			MIPS32_R_INST(17, 16, 0, src, dst, 6)
#define MIPS32_MTC0(gpr, cpr, sel)		MIPS32_R_INST(MIPS32_OP_COP0, MIPS32_COP0_MT, gpr, cpr, 0, sel)
#define MIPS32_MTC1(rt, fs)				MIPS32_R_INST(MIPS32_OP_COP1, MIPS32_COP1_MT, rt, fs, 0, 0)
#define MIPS32_CTC1(rt, fs)				MIPS32_R_INST(MIPS32_OP_COP1, MIPS32_COP1_CT, rt, fs, 0, 0)
#define MIPS32_LBU(reg, off, base)		MIPS32_I_INST(MIPS32_OP_LBU, base, reg, off)
#define MIPS32_LHU(reg, off, base)		MIPS32_I_INST(MIPS32_OP_LHU, base, reg, off)
#define MIPS32_LUI(reg, val)			MIPS32_I_INST(MIPS32_OP_LUI, 0, reg, val)
#define MIPS32_LW(reg, off, base)		MIPS32_I_INST(MIPS32_OP_LW, base, reg, off)
#define MIPS32_MFLO(reg)				MIPS32_R_INST(0, 0, 0, reg, 0, MIPS32_OP_MFLO)
#define MIPS32_MFHI(reg)				MIPS32_R_INST(0, 0, 0, reg, 0, MIPS32_OP_MFHI)
#define MIPS32_MTLO(reg)				MIPS32_R_INST(0, reg, 0, 0, 0, MIPS32_OP_MTLO)
#define MIPS32_MTHI(reg)				MIPS32_R_INST(0, reg, 0, 0, 0, MIPS32_OP_MTHI)
#define MIPS32_MUL(dst, src, t)			MIPS32_R_INST(28, src, t, dst, 0, MIPS32_OP_MUL)
#define MIPS32_OR(dst, src, val)		MIPS32_R_INST(0, src, val, dst, 0, 37)
#define MIPS32_ORI(tar, src, val)		MIPS32_I_INST(MIPS32_OP_ORI, src, tar, val)
#define MIPS32_XORI(tar, src, val)		MIPS32_I_INST(MIPS32_OP_XORI, src, tar, val)
#define MIPS32_RDHWR(tar, dst)			MIPS32_R_INST(MIPS32_OP_SPECIAL3, 0, tar, dst, 0, MIPS32_OP_RDHWR)
#define MIPS32_SB(reg, off, base)		MIPS32_I_INST(MIPS32_OP_SB, base, reg, off)
#define MIPS32_SH(reg, off, base)		MIPS32_I_INST(MIPS32_OP_SH, base, reg, off)
#define MIPS32_SW(reg, off, base)		MIPS32_I_INST(MIPS32_OP_SW, base, reg, off)
#define MIPS32_XOR(reg, val1, val2)		MIPS32_R_INST(0, val1, val2, reg, 0, MIPS32_OP_XOR)
#define MIPS32_SRA(reg, src, off)		MIPS32_R_INST(0, 0, src, reg, off, MIPS32_OP_SRL)
#define MIPS32_SRL(reg, src, off)		MIPS32_R_INST(0, 0, src, reg, off, MIPS32_OP_SRL)
#define MIPS32_SLTU(dst, src, tar)		MIPS32_R_INST(MIPS32_OP_SPECIAL, src, tar, dst, 0, MIPS32_OP_SLTU)
#define MIPS32_SLLV(dst, tar, src)		MIPS32_R_INST(MIPS32_OP_SPECIAL, src, tar, dst, 0, MIPS32_OP_SLLV)
#define MIPS32_SYNCI(off, base)			MIPS32_I_INST(MIPS32_OP_REGIMM, base, MIPS32_OP_SYNCI, off)
#define MIPS32_TLBR()					MIPS32_TLB_INST(MIPS32_OP_TLB, 1, 0, 1)

#define MIPS32_SYNC			0xF
#define MIPS32_SYNCI_STEP	0x1	/* reg num od address step size to be used with synci instruction */

/**
 * Cache operations definietions
 * Operation field is 5 bits long :
 * 1) bits 1..0 hold cache type
 * 2) bits 4..2 hold operation code
 */
#define MIPS32_CACHE_D_HIT_WRITEBACK ((0x1 << 0) | (0x6 << 2))
#define MIPS32_CACHE_I_HIT_INVALIDATE ((0x0 << 0) | (0x4 << 2))

/* ejtag specific instructions */
#define MIPS32_DRET					0x4200001F
#define MIPS32_SDBBP				0x7000003F	/* MIPS32_J_INST(MIPS32_OP_SPECIAL2, MIPS32_OP_SDBBP) */
#define MIPS16_SDBBP				0xE801
#define MICRO_MIPS32_SDBBP			0x000046C0
#define MICRO_MIPS_SDBBP			0x46C0
#define MIPS32_DSP_ENABLE			0x1000000

#define MIPS32_S_INST(rs, rac, opcode)			\
	(((rs) << 21) | ((rac) << 11) | (opcode))

#define MIPS32_DSP_R_INST(rt, immd, opcode, extrw) \
	((0x1F << 26) | ((immd) << 16) | ((rt) << 11) | ((opcode) << 6) | (extrw))
#define MIPS32_DSP_W_INST(rs, immd, opcode, extrw) \
	((0x1F << 26) | ((rs) << 21) | ((immd) << 11) | ((opcode) << 6) | (extrw))

#define MIPS32_DSP_MFHI(reg,ac)		MIPS32_R_INST(0, ac, 0, reg, 0, MIPS32_OP_MFHI)
#define MIPS32_DSP_MFLO(reg, ac)	MIPS32_R_INST(0, ac, 0, reg, 0, MIPS32_OP_MFLO)
#define MIPS32_DSP_MTLO(reg, ac)	MIPS32_S_INST(reg, ac, MIPS32_OP_MTLO)
#define MIPS32_DSP_MTHI(reg, ac)	MIPS32_S_INST(reg, ac, MIPS32_OP_MTHI)
#define MIPS32_DSP_RDDSP(rt, mask)	MIPS32_DSP_R_INST(rt, mask, 0x12, 0x38)
#define MIPS32_DSP_WRDSP(rs, mask)	MIPS32_DSP_W_INST(rs, mask, 0x13, 0x38)

//CPU types
//NOTE!	 If any cores added, must also add in mdi/mdi_internal.c
//CPU TYPES
#define MIPS_CORE_4K   0x0000
#define MIPS_CORE_4KE  0x0100
#define MIPS_CORE_4KS  0x0200
#define MIPS_CORE_5K   0x0400
#define MIPS_CORE_20K  0x0800
#define MIPS_CORE_M4K  0x1000
#define MIPS_CORE_24K  0x2000
#define MIPS_CORE_34K  0x4000
#define MIPS_CORE_AU1  0x8000
#define MIPS_CORE_24KE 0x10000
#define MIPS_CORE_74K  0x20000
#define MIPS_CORE_84K  0x40000
#define MIPS_CORE_BCM  0x80000
#define MIPS_CORE_1004K	 0x100000
#define MIPS_CORE_1074K	 0x200000
#define MIPS_CORE_M14K	  0x400000
#define MIPS_CORE_ALTERA  0x800000
#define MIPS_CORE_PROAPTIV	0x1000000
#define MIPS_CORE_INTERAPTIV  0x2000000
#define MIPS_CORE_5KE	0x4000000
#define MIPS_CORE_P5600 0x8000000
#define MIPS_CORE_I5500 0x10000000

#define MIPS_CORE_MASK 0xFFFFFF00
#define MIPS_VARIANT_MASK 0x00FF

//DEVICE: cpu information
//CPU
typedef enum {
   CPUTYPE_UNKNOWN = 0,
   MIPS_4Kc	  =0x0001 | MIPS_CORE_4K,
   MIPS_4Km	  =0x0002 | MIPS_CORE_4K,
   MIPS_4Kp	  =0x0004 | MIPS_CORE_4K,
   MIPS_4KEc  =0x0001 | MIPS_CORE_4KE,
   MIPS_4KEm  =0x0002 | MIPS_CORE_4KE,
   MIPS_4KEp  =0x0004 | MIPS_CORE_4KE,
   MIPS_4KSc  =0x0001 | MIPS_CORE_4KS,
   MIPS_4KSd  =0x0002 | MIPS_CORE_4KS,
   MIPS_M4K	  =0x0008 | MIPS_CORE_M4K,
   MIPS_24Kc  =0x0001 | MIPS_CORE_24K,
   MIPS_24Kf  =0x0010 | MIPS_CORE_24K,
   MIPS_24KEc =0x0001 | MIPS_CORE_24KE,
   MIPS_24KEf =0x0010 | MIPS_CORE_24KE,
   MIPS_34Kc  =0x0001 | MIPS_CORE_34K,
   MIPS_34Kf  =0x0010 | MIPS_CORE_34K,
   MIPS_5Kc	  =0x0001 | MIPS_CORE_5K,
   MIPS_5Kf	  =0x0010 | MIPS_CORE_5K,
   MIPS_5KEc  =0x0001 | MIPS_CORE_5KE,
   MIPS_5KEf  =0x0010 | MIPS_CORE_5KE,
   MIPS_20Kc  =0x0001 | MIPS_CORE_20K,
   MIPS_25Kf  =0x0010 | MIPS_CORE_20K,
   MIPS_AU1000=0x0001 | MIPS_CORE_AU1,
   MIPS_AU1100=0x0002 | MIPS_CORE_AU1,
   MIPS_AU1200=0x0003 | MIPS_CORE_AU1,
   MIPS_AU1500=0x0004 | MIPS_CORE_AU1,
   MIPS_AU1550=0x0005 | MIPS_CORE_AU1,
   MIPS_74Kc  =0x0001 | MIPS_CORE_74K,
   MIPS_74Kf  =0x0010 | MIPS_CORE_74K,
   MIPS_84Kc  =0x0001 | MIPS_CORE_84K,
   MIPS_84Kf  =0x0010 | MIPS_CORE_84K,
   MIPS_BCM	  =0x0000 | MIPS_CORE_BCM,
   MIPS_MP32   =0x0000 | MIPS_CORE_ALTERA,
   MIPS_1004Kc=0x0001 | MIPS_CORE_1004K,
   MIPS_1004Kf=0x0010 | MIPS_CORE_1004K,
   MIPS_1074Kc=0x0001 | MIPS_CORE_1074K,
   MIPS_1074Kf=0x0010 | MIPS_CORE_1074K,
   MIPS_M14Kc  =0x0001 | MIPS_CORE_M14K,
   MIPS_M14K   =0x0002 | MIPS_CORE_M14K,
   MIPS_M14Kf  =0x0010 | MIPS_CORE_M14K,
   MIPS_M14KE  =0x0020 | MIPS_CORE_M14K,	  // now called microAptiv UC
   MIPS_M14KEf =0x0030 | MIPS_CORE_M14K,	  // now called microAptiv UCF
   MIPS_M14KEc =0x0040 | MIPS_CORE_M14K,	  // now called microAptiv UP
   MIPS_M14KEcf=0x0050 | MIPS_CORE_M14K,	  // now called microAptiv UPF
   MIPS_M5100=0x0090 | MIPS_CORE_M14K,
   MIPS_M5150=0x00B0 | MIPS_CORE_M14K,
   MIPS_PROAPTIV =0x0001 | MIPS_CORE_PROAPTIV,
   MIPS_PROAPTIV_CM =0x0002 | MIPS_CORE_PROAPTIV,
   MIPS_INTERAPTIV =0x0001 | MIPS_CORE_INTERAPTIV,
   MIPS_INTERAPTIV_CM =0x0002 | MIPS_CORE_INTERAPTIV,
   MIPS_P5600 = MIPS_CORE_P5600,
   MIPS_I5500 = MIPS_CORE_I5500,
} CPUTYPE;

#define MMU_TLB 1
#define MMU_BAT 2
#define MMU_FIXED 3
#define MMU_DUAL_VTLB_FTLB 4

enum CPU_VENDOR {
	MIPS_CORE,
	ALCHEMY_CORE, 
	BROADCOM_CORE,
	ALTERA_CORE,
};

enum CPU_INSTRUCTION_SET {
	MIPS16,
	MIPS32,
	MIPS64,
	MICROMIPS_ONLY,
	MIPS32_AT_RESET_AND_MICROMIPS,
	MICROMIPS_AT_RESET_AND_MIPS32, 
};

enum EJTAG_VERSION {
	EJTAG_VER_UNKNOWN,
	EJTAG_2_0,
	EJTAG_2_5,
	EJTAG_2_6,
	EJTAG_3_1,
	EJTAG_4_0, 
	EJTAG_5_0,
};

typedef struct {
	uint32_t cpuCore;				// type of CPU	(4Kc, 24Kf, etc.)
	uint32_t cpuType;					// internal representation of cpu type
	enum CPU_VENDOR vendor;				// who makes the CPU:
	enum CPU_INSTRUCTION_SET instSet;	// MIPS16, MIPS32, microMIPS.
	uint32_t prid;						// processor's prid
	uint32_t numRegs;					//number of registers (same as calling HdiDeviceRegisterTotalGet)
	uint32_t numInstBkpts;				// number of instruction breakpoints in this CPU
	uint32_t numDataBkpts;				// number of data breakpoints in this CPU
	uint32_t numTcbTrig;				// number of TCB triggers in this CPU
	bool pdtrace;						// CPU has trace?
	bool asidInstBkpts;					// asid specification supported in Inst Bkpts?
	bool asidDataBkpts;					// asid specification supported in Data Bkpts?
	bool sharedInstBkpts;				// Are inst bkpts shared between VPE's
	bool sharedDataBkpts;				// Are data bkpts shared between VPE's
	bool armedInstBkpts;				// Do inst bkpts have armed triggering
	bool armedDataBkpts;				// Do inst bkpts have armed triggering
	uint32_t bkptRangePresent;			// Bitmask indicating which triggers have address ranging capability (bit 0-15 = inst, 16=31 = data)
	uint32_t tcbrev;					// TCB revision number
	uint32_t tcbCpuBits;				// Number of bits in the CPU field of trace words
	uint32_t tcbVmodes;					// Vmodes field (1=lsa supported, 2=lsad supported)
	bool pcTraceForcedOn;				// TRUE if hardware always collects PC trace
	bool mtase;							// CPU has MultiThreading extension?
	bool dspase;						// CPU has DSP extension?
	bool smase;							// CPU has SmartMIPS extension?
	bool m16ase;						// CPU has MIPS16[e] extension?
	bool micromipsase;					// CPU has microMIPS extension?
    uint32_t vzase;							// CPU has Virtualization ASE?
	uint32_t vzGuestIdWidth;
	bool vzGuestId;						// CPU has Virtualization ASE and supports Guest ID?
	bool profiling;						// Is profiling present?
	bool fpuPresent;					// CPU has floating point unit?
	bool pcSampSupported;				// CPU has PC Sampling capability
	bool DASampSupported;				// CPU has Data Address Sampling capability
	uint32_t cpuid;						// ebase.cpuid number
	uint32_t vpeid;						// VPE id number
	uint32_t numtc;						// Number of TC's in this processor
	uint32_t numvpe;					// Number of VPE's in this processor
	uint32_t numitc;					// Number of ITC cells in this processor
	bool offchip;						// Sofware supports off-chip trace?
	bool onchip;						// Sofware supports on-chip trace?
	bool cbtrig;						// CPU has complex break and trigger block?
	bool cbtrigPassCounters;			//CBT pass counters present?
	bool cbtrigTuples;					//CBT tuples present?
	bool cbtrigDataQualifiers;			//CBT data qualifiers present?
	bool cbtrigPrimedBreaks;			//CBT primed breaks present?
	bool cbtrigStopWatch;				//CBT stop watch present?
	bool cbtrigNot;						//CBT not (invert data value match) supported?
	bool pmtrace;						//Does system have performance monitor trace?
	uint32_t adsize;					//Address size
	enum EJTAG_VERSION ejtagVersion;	//EJTAG version
	uint32_t iCacheSize;
	uint32_t dCacheSize;
	uint32_t mmutype;
	uint32_t tlbEntries;
	uint32_t gtlbEntries;   // guest TLB
	bool fdcPresent;
	bool evaPresent;					// Enhanced virtual address (introduced with proAptiv).
	bool systemTracePresent;
	uint32_t numshadowregs;
	uint32_t impcode;
	uint32_t idcode;
	uint32_t onchipSize;
	bool cmPresent;
	bool msaPresent;
	bool msa;							// does cpu have MSA module
	bool mvh;							// are mfhc0 and mthc0 instructions implemented?
	bool guestCtl1Present;
	bool cdmm;
} CPU_INFO;

extern const struct command_registration mips32_command_handlers[];

int mips32_arch_state(struct target *target);
int mips32_read_cpu_config_info (struct target *target);

int mips32_init_arch_info(struct target *target,
		struct mips32_common *mips32, struct jtag_tap *tap);

int mips32_restore_context(struct target *target);
int mips32_save_context(struct target *target);

struct reg_cache *mips32_build_reg_cache(struct target *target);

int mips32_run_algorithm(struct target *target,
		int num_mem_params, struct mem_param *mem_params,
		int num_reg_params, struct reg_param *reg_params,
		uint32_t entry_point, uint32_t exit_point,
		int timeout_ms, void *arch_info);

int mips32_configure_break_unit(struct target *target);

int mips32_enable_interrupts(struct target *target, int enable);

int mips32_examine(struct target *target);

int mips32_register_commands(struct command_context *cmd_ctx);

int mips32_get_gdb_reg_list(struct target *target,
		struct reg **reg_list[], int *reg_list_size,
		enum target_register_class reg_class);
int mips32_checksum_memory(struct target *target, uint32_t address,
		uint32_t count, uint32_t *checksum);
int mips32_blank_check_memory(struct target *target,
		uint32_t address, uint32_t count, uint32_t *blank);

uint32_t DetermineCpuTypeFromPrid(uint32_t prid, uint32_t config, uint32_t config1);
int mips32_cp0_command(struct command_invocation *cmd);
int mips32_scan_delay_command(struct command_invocation *cmd);
#endif	/*MIPS32_H*/
