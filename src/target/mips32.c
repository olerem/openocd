/***************************************************************************
 *   Copyright (C) 2008 by Spencer Oliver                                  *
 *   spen@spen-soft.co.uk                                                  *
 *                                                                         *
 *   Copyright (C) 2008 by David T.L. Wong                                 *
 *                                                                         *
 *   Copyright (C) 2007,2008 Ã˜yvind Harboe                             *
 *   oyvind.harboe@zylin.com                                               *
 *                                                                         *
 *   Copyright (C) 2011 by Drasko DRASKOVIC                                *
 *   drasko.draskovic@gmail.com                                            *
 *                                                                         *
 *	 Copyright (C) 2014 by Kent Brinkley								   *
 *	 jkbrinkley.imgtec@gmail.com										   *
 *																		   *
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mips32.h"
#include "breakpoints.h"
#include "algorithm.h"
#include "register.h"

int mips_common_handle_target_request(void *priv);

static const char *mips_isa_strings[] = {
	"MIPS32", "MIPS16"
};

#define MIPS32_GDB_DUMMY_FP_REG 1

static const struct {
	unsigned option;
	const char *arg;
} invalidate_cmd[5] = {
	{ ALL, "all", },
	{ INST, "inst", },
	{ DATA, "data", },
	{ ALLNOWB, "allnowb", },
	{ DATANOWB, "datanowb", },
};

static const struct {
	unsigned id;
	const char *name;
	enum reg_type type;
	const char *group;
	const char *feature;
	int flag;
} mips32_regs[] = {
	{  0,  "r0", REG_TYPE_INT, NULL, "org.gnu.gdb.mips.cpu", 0 },
	{  1,  "r1", REG_TYPE_INT, NULL, "org.gnu.gdb.mips.cpu", 0 },
	{  2,  "r2", REG_TYPE_INT, NULL, "org.gnu.gdb.mips.cpu", 0 },
	{  3,  "r3", REG_TYPE_INT, NULL, "org.gnu.gdb.mips.cpu", 0 },
	{  4,  "r4", REG_TYPE_INT, NULL, "org.gnu.gdb.mips.cpu", 0 },
	{  5,  "r5", REG_TYPE_INT, NULL, "org.gnu.gdb.mips.cpu", 0 },
	{  6,  "r6", REG_TYPE_INT, NULL, "org.gnu.gdb.mips.cpu", 0 },
	{  7,  "r7", REG_TYPE_INT, NULL, "org.gnu.gdb.mips.cpu", 0 },
	{  8,  "r8", REG_TYPE_INT, NULL, "org.gnu.gdb.mips.cpu", 0 },
	{  9,  "r9", REG_TYPE_INT, NULL, "org.gnu.gdb.mips.cpu", 0 },
	{ 10, "r10", REG_TYPE_INT, NULL, "org.gnu.gdb.mips.cpu", 0 },
	{ 11, "r11", REG_TYPE_INT, NULL, "org.gnu.gdb.mips.cpu", 0 },
	{ 12, "r12", REG_TYPE_INT, NULL, "org.gnu.gdb.mips.cpu", 0 },
	{ 13, "r13", REG_TYPE_INT, NULL, "org.gnu.gdb.mips.cpu", 0 },
	{ 14, "r14", REG_TYPE_INT, NULL, "org.gnu.gdb.mips.cpu", 0 },
	{ 15, "r15", REG_TYPE_INT, NULL, "org.gnu.gdb.mips.cpu", 0 },
	{ 16, "r16", REG_TYPE_INT, NULL, "org.gnu.gdb.mips.cpu", 0 },
	{ 17, "r17", REG_TYPE_INT, NULL, "org.gnu.gdb.mips.cpu", 0 },
	{ 18, "r18", REG_TYPE_INT, NULL, "org.gnu.gdb.mips.cpu", 0 },
	{ 19, "r19", REG_TYPE_INT, NULL, "org.gnu.gdb.mips.cpu", 0 },
	{ 20, "r20", REG_TYPE_INT, NULL, "org.gnu.gdb.mips.cpu", 0 },
	{ 21, "r21", REG_TYPE_INT, NULL, "org.gnu.gdb.mips.cpu", 0 },
	{ 22, "r22", REG_TYPE_INT, NULL, "org.gnu.gdb.mips.cpu", 0 },
	{ 23, "r23", REG_TYPE_INT, NULL, "org.gnu.gdb.mips.cpu", 0 },
	{ 24, "r24", REG_TYPE_INT, NULL, "org.gnu.gdb.mips.cpu", 0 },
	{ 25, "r25", REG_TYPE_INT, NULL, "org.gnu.gdb.mips.cpu", 0 },
	{ 26, "r26", REG_TYPE_INT, NULL, "org.gnu.gdb.mips.cpu", 0 },
	{ 27, "r27", REG_TYPE_INT, NULL, "org.gnu.gdb.mips.cpu", 0 },
	{ 28, "r28", REG_TYPE_INT, NULL, "org.gnu.gdb.mips.cpu", 0 },
	{ 29, "r29", REG_TYPE_INT, NULL, "org.gnu.gdb.mips.cpu", 0 },
	{ 30, "r30", REG_TYPE_INT, NULL, "org.gnu.gdb.mips.cpu", 0 },
	{ 31, "r31", REG_TYPE_INT, NULL, "org.gnu.gdb.mips.cpu", 0 },
	{ 32, "status", REG_TYPE_INT, NULL, "org.gnu.gdb.mips.cp0", 0 },
	{ 33, "lo", REG_TYPE_INT, NULL, "org.gnu.gdb.mips.cpu", 0 },
	{ 34, "hi", REG_TYPE_INT, NULL, "org.gnu.gdb.mips.cpu", 0 },
	{ 35, "badvaddr", REG_TYPE_INT, NULL, "org.gnu.gdb.mips.cp0", 0 },
	{ 36, "cause", REG_TYPE_INT, NULL, "org.gnu.gdb.mips.cp0", 0 },
	{ 37, "pc", REG_TYPE_INT, NULL, "org.gnu.gdb.mips.cpu", 0 },

	{ 38,  "f0", REG_TYPE_IEEE_SINGLE, NULL,
		"org.gnu.gdb.mips.fpu", MIPS32_GDB_DUMMY_FP_REG },
	{ 39,  "f1", REG_TYPE_IEEE_SINGLE, NULL,
		"org.gnu.gdb.mips.fpu", MIPS32_GDB_DUMMY_FP_REG },
	{ 40,  "f2", REG_TYPE_IEEE_SINGLE, NULL,
		"org.gnu.gdb.mips.fpu", MIPS32_GDB_DUMMY_FP_REG },
	{ 41,  "f3", REG_TYPE_IEEE_SINGLE, NULL,
		"org.gnu.gdb.mips.fpu", MIPS32_GDB_DUMMY_FP_REG },
	{ 42,  "f4", REG_TYPE_IEEE_SINGLE, NULL,
		"org.gnu.gdb.mips.fpu", MIPS32_GDB_DUMMY_FP_REG },
	{ 43,  "f5", REG_TYPE_IEEE_SINGLE, NULL,
		"org.gnu.gdb.mips.fpu", MIPS32_GDB_DUMMY_FP_REG },
	{ 44,  "f6", REG_TYPE_IEEE_SINGLE, NULL,
		"org.gnu.gdb.mips.fpu", MIPS32_GDB_DUMMY_FP_REG },
	{ 45,  "f7", REG_TYPE_IEEE_SINGLE, NULL,
		"org.gnu.gdb.mips.fpu", MIPS32_GDB_DUMMY_FP_REG },
	{ 46,  "f8", REG_TYPE_IEEE_SINGLE, NULL,
		"org.gnu.gdb.mips.fpu", MIPS32_GDB_DUMMY_FP_REG },
	{ 47,  "f9", REG_TYPE_IEEE_SINGLE, NULL,
		"org.gnu.gdb.mips.fpu", MIPS32_GDB_DUMMY_FP_REG },
	{ 48, "f10", REG_TYPE_IEEE_SINGLE, NULL,
		"org.gnu.gdb.mips.fpu", MIPS32_GDB_DUMMY_FP_REG },
	{ 49, "f11", REG_TYPE_IEEE_SINGLE, NULL,
		"org.gnu.gdb.mips.fpu", MIPS32_GDB_DUMMY_FP_REG },
	{ 50, "f12", REG_TYPE_IEEE_SINGLE, NULL,
		"org.gnu.gdb.mips.fpu", MIPS32_GDB_DUMMY_FP_REG },
	{ 51, "f13", REG_TYPE_IEEE_SINGLE, NULL,
		"org.gnu.gdb.mips.fpu", MIPS32_GDB_DUMMY_FP_REG },
	{ 52, "f14", REG_TYPE_IEEE_SINGLE, NULL,
		"org.gnu.gdb.mips.fpu", MIPS32_GDB_DUMMY_FP_REG },
	{ 53, "f15", REG_TYPE_IEEE_SINGLE, NULL,
		"org.gnu.gdb.mips.fpu", MIPS32_GDB_DUMMY_FP_REG },
	{ 54, "f16", REG_TYPE_IEEE_SINGLE, NULL,
		"org.gnu.gdb.mips.fpu", MIPS32_GDB_DUMMY_FP_REG },
	{ 55, "f17", REG_TYPE_IEEE_SINGLE, NULL,
		"org.gnu.gdb.mips.fpu", MIPS32_GDB_DUMMY_FP_REG },
	{ 56, "f18", REG_TYPE_IEEE_SINGLE, NULL,
		"org.gnu.gdb.mips.fpu", MIPS32_GDB_DUMMY_FP_REG },
	{ 57, "f19", REG_TYPE_IEEE_SINGLE, NULL,
		"org.gnu.gdb.mips.fpu", MIPS32_GDB_DUMMY_FP_REG },
	{ 58, "f20", REG_TYPE_IEEE_SINGLE, NULL,
		"org.gnu.gdb.mips.fpu", MIPS32_GDB_DUMMY_FP_REG },
	{ 59, "f21", REG_TYPE_IEEE_SINGLE, NULL,
		"org.gnu.gdb.mips.fpu", MIPS32_GDB_DUMMY_FP_REG },
	{ 60, "f22", REG_TYPE_IEEE_SINGLE, NULL,
		"org.gnu.gdb.mips.fpu", MIPS32_GDB_DUMMY_FP_REG },
	{ 61, "f23", REG_TYPE_IEEE_SINGLE, NULL,
		"org.gnu.gdb.mips.fpu", MIPS32_GDB_DUMMY_FP_REG },
	{ 62, "f24", REG_TYPE_IEEE_SINGLE, NULL,
		"org.gnu.gdb.mips.fpu", MIPS32_GDB_DUMMY_FP_REG },
	{ 63, "f25", REG_TYPE_IEEE_SINGLE, NULL,
		"org.gnu.gdb.mips.fpu", MIPS32_GDB_DUMMY_FP_REG },
	{ 64, "f26", REG_TYPE_IEEE_SINGLE, NULL,
		"org.gnu.gdb.mips.fpu", MIPS32_GDB_DUMMY_FP_REG },
	{ 65, "f27", REG_TYPE_IEEE_SINGLE, NULL,
		"org.gnu.gdb.mips.fpu", MIPS32_GDB_DUMMY_FP_REG },
	{ 66, "f28", REG_TYPE_IEEE_SINGLE, NULL,
		"org.gnu.gdb.mips.fpu", MIPS32_GDB_DUMMY_FP_REG },
	{ 67, "f29", REG_TYPE_IEEE_SINGLE, NULL,
		"org.gnu.gdb.mips.fpu", MIPS32_GDB_DUMMY_FP_REG },
	{ 68, "f30", REG_TYPE_IEEE_SINGLE, NULL,
		"org.gnu.gdb.mips.fpu", MIPS32_GDB_DUMMY_FP_REG },
	{ 69, "f31", REG_TYPE_IEEE_SINGLE, NULL,
		"org.gnu.gdb.mips.fpu", MIPS32_GDB_DUMMY_FP_REG },
	{ 70, "fcsr", REG_TYPE_INT, "float",
		"org.gnu.gdb.mips.fpu", MIPS32_GDB_DUMMY_FP_REG },
	{ 71, "fir", REG_TYPE_INT, "float",
		"org.gnu.gdb.mips.fpu", MIPS32_GDB_DUMMY_FP_REG },
};

/* Order fixed */
static const struct {
	const char *name;
} mips32_dsp_regs[MIPS32NUMDSPREGS] = {
	{ "hi0"},
	{ "hi1"},
	{ "hi2"},
	{ "hi3"},
	{ "lo0"},
	{ "lo1"},
	{ "lo2"},
	{ "lo3"},
	{ "control"},
};

/* number of mips dummy fp regs fp0 - fp31 + fsr and fir
 * we also add 18 unknown registers to handle gdb requests */

#define MIPS32_NUM_REGS ARRAY_SIZE(mips32_regs)

/* WAYS MAPPING */
static const int wayTable[] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};	   /* field->ways mapping */
static const int setTableISDS[] = {64,128,256,512,1024,2048,4096,32,		  /* field->sets mapping */
16*1024, 32*1024, 64*1024, 128*1024, 256*1024, 512*1024, 1024*1024, 2048*1024};
static const int setTable[] = {64,128,256,512,1024,2048,4096,8192,			  /* field->sets mapping */
16*1024, 32*1024, 64*1024, 128*1024, 256*1024, 512*1024, 1024*1024, 2048*1024};

/* BPL */
static const int bplTable[] = {0,4,8,16,32,64,128,256,512,1024,2048,4*1024,8*1024,16*1024,32*1024,64*1024}; /* field->bytes per line */
static const int bplbitTable[] = {0,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};									/* field->bits in bpl */

static uint32_t  ftlb_ways[16] = {2,3,4,5, 6,7,8,0,   0,0,0,0, 0,0,0,0};
static uint32_t  ftlb_sets[16] = {1,2,4,8, 16,32,64,128, 256,0,0,0, 0,0,0,0};

static int mips32_get_core_reg(struct reg *reg)
{
	int retval;
	struct mips32_core_reg *mips32_reg = reg->arch_info;
	struct target *target = mips32_reg->target;
	struct mips32_common *mips32_target = target_to_mips32(target);

	if (target->state != TARGET_HALTED)
		return ERROR_TARGET_NOT_HALTED;

	retval = mips32_target->read_core_reg(target, mips32_reg->num);
	return retval;
}

static int mips32_set_core_reg(struct reg *reg, uint8_t *buf)
{
	struct mips32_core_reg *mips32_reg = reg->arch_info;
	struct target *target = mips32_reg->target;

	/* get pointers to arch-specific information */
	struct mips32_common *mips32 = target_to_mips32(target);

	if ((mips32->fp_implemented != FP_IMP)&& (mips32_reg->num > MIPS32_PC)){
		LOG_USER ("No Upate - No FPU Available");
		return ERROR_OK;
	}

	uint32_t value = buf_get_u32(buf, 0, 32);

	if (target->state != TARGET_HALTED)
		return ERROR_TARGET_NOT_HALTED;

	buf_set_u32(reg->value, 0, 32, value);
	reg->dirty = 1;
	reg->valid = 1;

	return ERROR_OK;
}

static int mips32_read_core_reg(struct target *target, unsigned int num)
{
	uint32_t reg_value;

	/* get pointers to arch-specific information */
	struct mips32_common *mips32 = target_to_mips32(target);

	if (num >= MIPS32_NUM_REGS)
		return ERROR_COMMAND_SYNTAX_ERROR;

	reg_value = mips32->core_regs[num];
	buf_set_u32(mips32->core_cache->reg_list[num].value, 0, 32, reg_value);
	mips32->core_cache->reg_list[num].valid = 1;
	mips32->core_cache->reg_list[num].dirty = 0;

	return ERROR_OK;
}

static int mips32_write_core_reg(struct target *target, unsigned int num)
{
	uint32_t reg_value;

	/* get pointers to arch-specific information */
	struct mips32_common *mips32 = target_to_mips32(target);

	if (num >= MIPS32_NUM_REGS)
		return ERROR_COMMAND_SYNTAX_ERROR;

	if ((mips32->fp_implemented != FP_IMP)&& (num > MIPS32_PC)){
		LOG_USER ("No FPU Available");
	} else {
		reg_value = buf_get_u32(mips32->core_cache->reg_list[num].value, 0, 32);
		mips32->core_regs[num] = reg_value;
		LOG_DEBUG("write core reg %i value 0x%" PRIx32 "", num , reg_value);
		mips32->core_cache->reg_list[num].valid = 1;
		mips32->core_cache->reg_list[num].dirty = 0;
	}

	return ERROR_OK;
}

int mips32_get_gdb_reg_list(struct target *target, struct reg **reg_list[],
		int *reg_list_size, enum target_register_class reg_class)
{
	/* get pointers to arch-specific information */
	struct mips32_common *mips32 = target_to_mips32(target);
	unsigned int i;

	/* include floating point registers */
	*reg_list_size = MIPS32_NUM_REGS;
	*reg_list = malloc(sizeof(struct reg *) * (*reg_list_size));

	for (i = 0; i < MIPS32_NUM_REGS; i++)
		(*reg_list)[i] = &mips32->core_cache->reg_list[i];

	return ERROR_OK;
}

int mips32_save_context(struct target *target)
{
	unsigned int i;
	int retval;
	uint32_t config1;

	/* get pointers to arch-specific information */
	struct mips32_common *mips32 = target_to_mips32(target);
	struct mips_ejtag *ejtag_info = &mips32->ejtag_info;

	/* read core registers */
	retval = mips32_pracc_read_regs(ejtag_info, mips32->core_regs);
	if (retval != ERROR_OK) {
		LOG_DEBUG("mips32_pracc_read_regs failed");
		return retval;
	}

	for (i = 0; i < MIPS32_NUM_REGS; i++) {
		if (!mips32->core_cache->reg_list[i].valid) {
			retval = mips32->read_core_reg(target, i);
			if (retval != ERROR_OK) {
				LOG_DEBUG("mips32->read_core_reg failed");
				return retval;
			}
		}
	}

	/* Read Config1 registers */
	retval = mips32_pracc_cp0_read(ejtag_info, &config1, 16, 1);
	if (retval != ERROR_OK) {
		LOG_DEBUG("reading config3 register failed");
		return retval;
	}

	/* Retrive if Float Point CoProcessor Implemented */
	mips32->fp_implemented = (config1 & CFG1_FP);

	/* FP Coprocessor available read FP registers */
	if (mips32->fp_implemented == FP_IMP) {
		uint32_t config3;
		uint32_t mvpconf1;
		uint32_t status;
		uint32_t tmp_status;

		/* Read Config3 registers */
		retval = mips32_pracc_cp0_read(ejtag_info, &config3, 16, 3);
		if (retval != ERROR_OK) {
			LOG_DEBUG("reading config3 register failed");
			return retval;
		}

		/* Read mvpconf1 registers */
		retval = mips32_pracc_cp0_read(ejtag_info, &mvpconf1, 0, 3);
		if (retval != ERROR_OK) {
			LOG_DEBUG("reading config3 register failed");
			return retval;
		}

		/* check if VPE has access to FPU */
		if ((config1 & 0x00000001) ) {

			/* Check if multi-thread core with single thread FPU */
			if ((((config3 & 0x00000004) >> 2) == 1) && ((mvpconf1 & 0x00000001) == 1)) {

				/* Read Status register, save it and modify to enable CP0 */
				retval = mips32_pracc_cp0_read(ejtag_info, &status, 12, 0);
				if (retval != ERROR_OK) {
					LOG_DEBUG("reading status register failed");
					return retval;
				}

				/* Check if Access to COP1 enabled */
				if (((status & 0x20000000) >> 29) == 0) {
					if ((retval = mips32_pracc_cp0_write(ejtag_info, (status | STATUS_CU1_MASK), 12, 0)) != ERROR_OK) {
						LOG_DEBUG("writing status register failed");
						return retval;
					}

					/* Verify CP1 Enabled */
					retval = mips32_pracc_cp0_read(ejtag_info, &tmp_status, 12, 0);
					if (retval != ERROR_OK) {
						LOG_DEBUG("writing status register failed");
						return retval;
					}

					if (((tmp_status & 0x20000000) >> 29)!= 1) {
						LOG_USER ("1 Access to FPU not available - tmp_status: 0x%x, status: 0x%x", tmp_status, status);
						return retval;
					}
				}

				/* read core registers */
				retval = mips32_pracc_read_fpu_regs(ejtag_info, (uint32_t *)(&mips32->core_regs[38]));
				if (retval != ERROR_OK)
					LOG_INFO("mips32->read_core_reg failed");

				/* restore previous setting */
				if ((mips32_pracc_cp0_write(ejtag_info, status, 12, 0)) != ERROR_OK)
					LOG_DEBUG("writing status register failed");
			}
		} else {
//			LOG_INFO("Enable COP2");
			/* Read Status register, save it and modify to enable CP0 */
			retval = mips32_pracc_cp0_read(ejtag_info, &status, 12, 0);
			if (retval != ERROR_OK) {
				LOG_DEBUG("reading status register failed");
				return retval;
			}

			/* Check if Access to COP1 enabled */
			if (((status & 0x20000000) >> 29) == 0) {
				if ((retval = mips32_pracc_cp0_write(ejtag_info, (status | STATUS_CU1_MASK), 12, 0)) != ERROR_OK) {
					LOG_DEBUG("writing status register failed");
						return retval;
				}
			}

			/* read core registers */
			retval = mips32_pracc_read_fpu_regs(ejtag_info, (uint32_t *)(&mips32->core_regs[38]));
			if (retval != ERROR_OK)
				LOG_INFO("mips32->read_fpu_reg failed: status: 0x%x, tmp_status: 0x%x", status, tmp_status);
			
			/* restore previous setting */
			if ((mips32_pracc_cp0_write(ejtag_info, status, 12, 0)) != ERROR_OK)
				LOG_DEBUG("writing status register failed");
		}

		for (i = 38; i < MIPS32_NUM_REGS; i++) {
			if (mips32->core_cache->reg_list[i].valid) {
				retval = mips32->read_core_reg(target, i);
				if (retval != ERROR_OK) {
					LOG_DEBUG("mips32->read_core_reg failed");
					return retval;
				}
			}
		}

	}

	return ERROR_OK;
}

int mips32_restore_context(struct target *target)
{
	unsigned int i;
	int retval;
	uint32_t config1;

	/* get pointers to arch-specific information */
	struct mips32_common *mips32 = target_to_mips32(target);
	struct mips_ejtag *ejtag_info = &mips32->ejtag_info;

	for (i = 0; i < MIPS32_NUM_REGS; i++) {
		if (mips32->core_cache->reg_list[i].dirty)
			mips32->write_core_reg(target, i);
	}

	/* If FPU then Update registers */
	/* FP Coprocessor available read FP registers */
	if (mips32->fp_implemented == FP_IMP) {
		uint32_t config3;
		uint32_t mvpconf1;
		uint32_t status;
		uint32_t tmp_status;

		/* Read Config1 registers */
		retval = mips32_pracc_cp0_read(ejtag_info, &config1, 16, 3);
		if (retval != ERROR_OK) {
			LOG_DEBUG("reading config3 register failed");
			return retval;
		}

		/* Read Config3 registers */
		retval = mips32_pracc_cp0_read(ejtag_info, &config3, 16, 3);
		if (retval != ERROR_OK) {
			LOG_DEBUG("reading config3 register failed");
			return retval;
		}

		/* Read mvpconf1 registers */
		retval = mips32_pracc_cp0_read(ejtag_info, &mvpconf1, 0, 3);
		if (retval != ERROR_OK) {
			LOG_DEBUG("reading config3 register failed");
			return retval;
		}

		/* check if FPU configured and Multi-thread core*/
		if ((config1 & 0x00000001) == 1) {

			/* Check if multi-thread core with single thread FPU */
			if ((((config3 & 0x00000004) >> 2) == 1) && ((mvpconf1 & 0x00000001) == 1)) {

				/* Read Status register, save it and modify to enable CP0 */
				retval = mips32_pracc_cp0_read(ejtag_info, &status, 12, 0);
				if (retval != ERROR_OK) {
					LOG_DEBUG("reading status register failed");
					return retval;
				}

				if ((retval = mips32_pracc_cp0_write(ejtag_info, (status | STATUS_CU1_MASK), 12, 0)) != ERROR_OK) {
					LOG_DEBUG("writing status register failed");
					return retval;
				}

				/* Verify CP1 Enabled */
				retval = mips32_pracc_cp0_read(ejtag_info, &tmp_status, 12, 0);
				if (retval != ERROR_OK) {
					LOG_DEBUG("writing status register failed");
					return retval;
				}

				if (((tmp_status & 0x20000000) >> 29)!= 1) {
					LOG_USER ("1 Access to FPU not available - tmp_status: 0x%x, status: 0x%x", tmp_status, status);
					return retval;
				}

				/* read core registers */
				retval = mips32_pracc_write_fpu_regs(ejtag_info, (uint32_t *)(&mips32->core_regs[38]));
				if (retval != ERROR_OK)
					LOG_INFO("mips32->read_core_reg failed");

				/* restore previous setting */
				if ((mips32_pracc_cp0_write(ejtag_info, status, 12, 0)) != ERROR_OK)
					LOG_DEBUG("writing status register failed");
			} else {
				/* Read Status register, save it and modify to enable CP0 */
				retval = mips32_pracc_cp0_read(ejtag_info, &status, 12, 0);
				if (retval != ERROR_OK) {
					LOG_DEBUG("reading status register failed");
					return retval;
				}

				if ((retval = mips32_pracc_cp0_write(ejtag_info, (status | STATUS_CU1_MASK), 12, 0)) != ERROR_OK) {
					LOG_DEBUG("writing status register failed");
					return retval;
				}


				/* read core registers */
				retval = mips32_pracc_write_fpu_regs(ejtag_info, (uint32_t *)(&mips32->core_regs[38]));
				if (retval != ERROR_OK)
					LOG_INFO("mips32->read_core_reg failed");

				/* restore previous setting */
				if ((mips32_pracc_cp0_write(ejtag_info, status, 12, 0)) != ERROR_OK)
					LOG_DEBUG("writing status register failed");
			}
		}
	}

	/* write core regs */
	mips32_pracc_write_regs(ejtag_info, mips32->core_regs);

	return ERROR_OK;
}

int mips32_arch_state(struct target *target)
{
	struct mips32_common *mips32 = target_to_mips32(target);

	/* Read interesting Configuration Registers */
	mips32_read_cpu_config_info (target);

	LOG_USER("target halted in %s mode due to %s, pc: 0x%8.8" PRIx32 "",
		mips_isa_strings[mips32->isa_mode],
		debug_reason_name(target),
		buf_get_u32(mips32->core_cache->reg_list[MIPS32_PC].value, 0, 32));

	return ERROR_OK;
}

int mips32_read_cpu_config_info (struct target *target)
{
	struct mips32_common *mips32 = target_to_mips32(target);
	struct mips_ejtag *ejtag_info = &mips32->ejtag_info;

	int retval;
	uint32_t	prid; /* cp0 PRID - 15, 0 */
	uint32_t config;
	uint32_t config3;
	uint32_t config1;
	uint32_t config23;
	uint32_t dcr;

	/* Read PRID registers */
	if ((retval = mips32_pracc_cp0_read(ejtag_info, &prid, 15, 0)) != ERROR_OK) {
		LOG_DEBUG("READ of PRID Failed");
		return retval;
	}

	/* Read Config register */
	if ((retval = mips32_pracc_cp0_read(ejtag_info, &config, 16, 0))!= ERROR_OK) {
		LOG_DEBUG("Read of Config reg Failed");
		return retval;
	}

	/* Read Config1 register */
	if ((retval = mips32_pracc_cp0_read(ejtag_info, &config1, 16, 1))!= ERROR_OK) {
		LOG_DEBUG("Read of Config1 read Failed");
		return retval;
	}

	/* Read debug(config 32, 0 register */
	if ((retval = mips32_pracc_cp0_read(ejtag_info, &config23, 23, 0))!= ERROR_OK) {
		LOG_DEBUG("Read of Config1 read Failed");
		return retval;
	} else
		LOG_DEBUG("Debug register: 0x%8.8x", config23);

	/* Read Config3 register */
	retval = mips32_pracc_cp0_read(ejtag_info, &config3, 16, 3);
	if (retval != ERROR_OK) {
		LOG_DEBUG("reading config3 register failed");
		return retval;
	}

	/* Retrive if Float Point CoProcessor Implemented */
	mips32->fp_implemented = (config1 & CFG1_FP);

	/* Retrieve DSP info */
	mips32->dsp_implemented = ((config3 & CFG3_DSPP) >>  10);
	mips32->dsp_rev = ((config3 & CFG3_DSP_REV) >>  11);

	/* Retrieve ISA Mode */
	mips32->mmips = ((config3 & CFG3_ISA_MODE) >>  14);

	/* Determine if FDC and CDMM are implemented for this core */
	if ((retval = target_read_u32(target, EJTAG_DCR, &dcr)) != ERROR_OK)
		return retval;

	if ((((dcr > 18) & 0x1) == 1) && ((config3 & 0x00000008) != 0)) {
		retval = target_register_timer_callback(mips_common_handle_target_request, 3, 3, target);
		if (retval != ERROR_OK)
			return retval;

		mips32->fdc = 1;
		mips32->semihosting = ENABLE_SEMIHOSTING;
	}
	else {
		mips32->fdc = 0;
		mips32->semihosting = DISABLE_SEMIHOSTING;
	}

	uint32_t cputype = DetermineCpuTypeFromPrid(prid, config, config1);

	/* determine whether uC or uP core */
	if ((cputype == MIPS_M14KE) || (cputype == MIPS_M14KEf))
		mips32->cp0_mask = MIPS_CP0_mAPTIV_uC;
	else
		if ((cputype == MIPS_M14KEc) || (cputype == MIPS_M14KEcf))
			mips32->cp0_mask = MIPS_CP0_mAPTIV_uP;

	return ERROR_OK;
}

static const struct reg_arch_type mips32_reg_type = {
	.get = mips32_get_core_reg,
	.set = mips32_set_core_reg,
};

struct reg_cache *mips32_build_reg_cache(struct target *target)
{
	/* get pointers to arch-specific information */
	struct mips32_common *mips32 = target_to_mips32(target);

	int num_regs = MIPS32_NUM_REGS;
	struct reg_cache **cache_p = register_get_last_cache_p(&target->reg_cache);
	struct reg_cache *cache = malloc(sizeof(struct reg_cache));
	struct reg *reg_list = calloc(num_regs, sizeof(struct reg));
	struct mips32_core_reg *arch_info = malloc(sizeof(struct mips32_core_reg) * num_regs);
	struct reg_feature *feature;
	int i;

	/* Build the process context cache */
	cache->name = "mips32 registers";
	cache->next = NULL;
	cache->reg_list = reg_list;
	cache->num_regs = num_regs;
	(*cache_p) = cache;
	mips32->core_cache = cache;

	for (i = 0; i < num_regs; i++) {
		arch_info[i].num = mips32_regs[i].id;
		arch_info[i].target = target;
		arch_info[i].mips32_common = mips32;

		reg_list[i].name = mips32_regs[i].name;
		reg_list[i].size = 32;

		reg_list[i].value = calloc(1, 4);
		reg_list[i].valid = 0;
		reg_list[i].type = &mips32_reg_type;
		reg_list[i].arch_info = &arch_info[i];
		reg_list[i].reg_data_type = calloc(1, sizeof(struct reg_data_type));
		if (reg_list[i].reg_data_type)
			reg_list[i].reg_data_type->type = mips32_regs[i].type;
		else
			LOG_ERROR("unable to allocate reg type list");

		reg_list[i].dirty = 0;
		reg_list[i].group = mips32_regs[i].group;
		reg_list[i].number = i;
		reg_list[i].exist = true;
		reg_list[i].caller_save = true;	/* gdb defaults to true */
		feature = calloc(1, sizeof(struct reg_feature));
		if (feature) {
			feature->name = mips32_regs[i].feature;
			reg_list[i].feature = feature;
		} else
			LOG_ERROR("unable to allocate feature list");
	}

	return cache;
}


int mips32_init_arch_info(struct target *target, struct mips32_common *mips32, struct jtag_tap *tap)
{
	target->arch_info = mips32;
	mips32->common_magic = MIPS32_COMMON_MAGIC;
	mips32->fast_data_area = NULL;
	int retval;

	/* has breakpoint/watchpint unit been scanned */
	mips32->bp_scanned = 0;
	mips32->data_break_list = NULL;

	mips32->ejtag_info.tap = tap;
	mips32->read_core_reg = mips32_read_core_reg;
	mips32->write_core_reg = mips32_write_core_reg;

	mips32->ejtag_info.scan_delay = MIPS32_SCAN_DELAY_LEGACY_MODE;	/* Initial default value */
	mips32->ejtag_info.mode = 0;			/* Initial default value */
	mips32->fdc = -1;

	return ERROR_OK;
}

/* run to exit point. return error if exit point was not reached. */
static int mips32_run_and_wait(struct target *target, uint32_t entry_point,
		int timeout_ms, uint32_t exit_point, struct mips32_common *mips32)
{
	uint32_t pc;
	int retval;
	/* This code relies on the target specific  resume() and  poll()->debug_entry()
	 * sequence to write register values to the processor and the read them back */
	retval = target_resume(target, 0, entry_point, 0, 1);
	if (retval != ERROR_OK)
		return retval;

	retval = target_wait_state(target, TARGET_HALTED, timeout_ms);
	/* If the target fails to halt due to the breakpoint, force a halt */
	if (retval != ERROR_OK || target->state != TARGET_HALTED) {
		retval = target_halt(target);
		if (retval != ERROR_OK)
			return retval;
		retval = target_wait_state(target, TARGET_HALTED, 500);
		if (retval != ERROR_OK)
			return retval;
		return ERROR_TARGET_TIMEOUT;
	}

	pc = buf_get_u32(mips32->core_cache->reg_list[MIPS32_PC].value, 0, 32);
	if (exit_point && (pc != exit_point)) {
		LOG_DEBUG("failed algorithm halted at 0x%" PRIx32 " ", pc);
		return ERROR_TARGET_TIMEOUT;
	}

	return ERROR_OK;
}

int mips32_run_algorithm(struct target *target, int num_mem_params,
		struct mem_param *mem_params, int num_reg_params,
		struct reg_param *reg_params, uint32_t entry_point,
		uint32_t exit_point, int timeout_ms, void *arch_info)
{
	struct mips32_common *mips32 = target_to_mips32(target);
	struct mips32_algorithm *mips32_algorithm_info = arch_info;
	enum mips32_isa_mode isa_mode = mips32->isa_mode;

	uint32_t context[MIPS32_NUM_REGS];
	unsigned int i;
	int retval = ERROR_OK;

	LOG_DEBUG("Running algorithm");

	/* NOTE: mips32_run_algorithm requires that each algorithm uses a software breakpoint
	 * at the exit point */

	if (mips32->common_magic != MIPS32_COMMON_MAGIC) {
		LOG_ERROR("current target isn't a MIPS32 target");
		return ERROR_TARGET_INVALID;
	}

	if (target->state != TARGET_HALTED) {
		LOG_WARNING("target not halted");
		return ERROR_TARGET_NOT_HALTED;
	}

	/* refresh core register cache */
	for (i = 0; i < MIPS32_NUM_REGS; i++) {
		if (!mips32->core_cache->reg_list[i].valid)
			mips32->read_core_reg(target, i);
		context[i] = buf_get_u32(mips32->core_cache->reg_list[i].value, 0, 32);
	}

	for (i = 0; i < (unsigned int)num_mem_params; i++) {
		retval = target_write_buffer(target, mem_params[i].address,
				mem_params[i].size, mem_params[i].value);
		if (retval != ERROR_OK)
			return retval;
	}

	for (i = 0; i < (unsigned int)num_reg_params; i++) {
		struct reg *reg = register_get_by_name(mips32->core_cache, reg_params[i].reg_name, 0);

		if (!reg) {
			LOG_ERROR("BUG: register '%s' not found", reg_params[i].reg_name);
			return ERROR_COMMAND_SYNTAX_ERROR;
		}

		if (reg->size != reg_params[i].size) {
			LOG_ERROR("BUG: register '%s' size doesn't match reg_params[i].size",
					reg_params[i].reg_name);
			return ERROR_COMMAND_SYNTAX_ERROR;
		}

		mips32_set_core_reg(reg, reg_params[i].value);
	}

	mips32->isa_mode = mips32_algorithm_info->isa_mode;

	retval = mips32_run_and_wait(target, entry_point, timeout_ms, exit_point, mips32);

	if (retval != ERROR_OK)
		return retval;

	for (i = 0; i < (unsigned int)num_mem_params; i++) {
		if (mem_params[i].direction != PARAM_OUT) {
			retval = target_read_buffer(target, mem_params[i].address, mem_params[i].size,
					mem_params[i].value);
			if (retval != ERROR_OK)
				return retval;
		}
	}

	for (i = 0; i < (unsigned int)num_reg_params; i++) {
		if (reg_params[i].direction != PARAM_OUT) {
			struct reg *reg = register_get_by_name(mips32->core_cache, reg_params[i].reg_name, 0);
			if (!reg) {
				LOG_ERROR("BUG: register '%s' not found", reg_params[i].reg_name);
				return ERROR_COMMAND_SYNTAX_ERROR;
			}

			if (reg->size != reg_params[i].size) {
				LOG_ERROR("BUG: register '%s' size doesn't match reg_params[i].size",
						reg_params[i].reg_name);
				return ERROR_COMMAND_SYNTAX_ERROR;
			}

			buf_set_u32(reg_params[i].value, 0, 32, buf_get_u32(reg->value, 0, 32));
		}
	}

	/* restore everything we saved before */
	for (i = 0; i < MIPS32_NUM_REGS; i++) {
		uint32_t regvalue;
		regvalue = buf_get_u32(mips32->core_cache->reg_list[i].value, 0, 32);
		if (regvalue != context[i]) {
			LOG_DEBUG("restoring register %s with value 0x%8.8" PRIx32,
				mips32->core_cache->reg_list[i].name, context[i]);
			buf_set_u32(mips32->core_cache->reg_list[i].value,
					0, 32, context[i]);
			mips32->core_cache->reg_list[i].valid = 1;
			mips32->core_cache->reg_list[i].dirty = 1;
		}
	}

	mips32->isa_mode = isa_mode;

	return ERROR_OK;
}

int mips32_examine(struct target *target)
{
	struct mips32_common *mips32 = target_to_mips32(target);
	if (!target_was_examined(target)) {
		target_set_examined(target);

		/* we will configure later */
		mips32->bp_scanned = 0;
		mips32->num_inst_bpoints = 0;
		mips32->num_data_bpoints = 0;
		mips32->num_inst_bpoints_avail = 0;
		mips32->num_data_bpoints_avail = 0;
	}

	return ERROR_OK;
}

static int mips32_configure_ibs(struct target *target)
{
	struct mips32_common *mips32 = target_to_mips32(target);
	struct mips_ejtag *ejtag_info = &mips32->ejtag_info;
	int retval, i;
	uint32_t bpinfo;

	/* get number of inst breakpoints */
	retval = target_read_u32(target, ejtag_info->ejtag_ibs_addr, &bpinfo);
	if (retval != ERROR_OK)
		return retval;

	mips32->num_inst_bpoints = (bpinfo >> 24) & 0x0F;
	mips32->num_inst_bpoints_avail = mips32->num_inst_bpoints;
	mips32->inst_break_list = calloc(mips32->num_inst_bpoints,
		sizeof(struct mips32_comparator));

	for (i = 0; i < mips32->num_inst_bpoints; i++)
		mips32->inst_break_list[i].reg_address =
			ejtag_info->ejtag_iba0_addr +
			(ejtag_info->ejtag_iba_step_size * i);

	/* clear IBIS reg */
	retval = target_write_u32(target, ejtag_info->ejtag_ibs_addr, 0);
	return retval;
}

static int mips32_configure_dbs(struct target *target)
{
	struct mips32_common *mips32 = target_to_mips32(target);
	struct mips_ejtag *ejtag_info = &mips32->ejtag_info;
	int retval, i;
	uint32_t bpinfo;

	/* get number of data breakpoints */
	retval = target_read_u32(target, ejtag_info->ejtag_dbs_addr, &bpinfo);
	if (retval != ERROR_OK)
		return retval;

	mips32->num_data_bpoints = (bpinfo >> 24) & 0x0F;
	mips32->num_data_bpoints_avail = mips32->num_data_bpoints;
	mips32->data_break_list = calloc(mips32->num_data_bpoints,
		sizeof(struct mips32_comparator));

	for (i = 0; i < mips32->num_data_bpoints; i++)
		mips32->data_break_list[i].reg_address =
			ejtag_info->ejtag_dba0_addr +
			(ejtag_info->ejtag_dba_step_size * i);

	/* clear DBIS reg */
	retval = target_write_u32(target, ejtag_info->ejtag_dbs_addr, 0);
	return retval;
}

int mips32_configure_break_unit(struct target *target)
{
	/* get pointers to arch-specific information */
	struct mips32_common *mips32 = target_to_mips32(target);
	struct mips_ejtag *ejtag_info = &mips32->ejtag_info;
	int retval;
	uint32_t dcr;

	if (mips32->bp_scanned)
		return ERROR_OK;

	/* get info about breakpoint support */
	retval = target_read_u32(target, EJTAG_DCR, &dcr);
	if (retval != ERROR_OK)
		return retval;

	/* EJTAG 2.0 defines IB and DB bits in IMP instead of DCR. */
	if (ejtag_info->ejtag_version == EJTAG_VERSION_20) {
		ejtag_info->debug_caps = dcr & EJTAG_DCR_ENM;
		if (!(ejtag_info->impcode & EJTAG_V20_IMP_NOIB))
			ejtag_info->debug_caps |= EJTAG_DCR_IB;
		if (!(ejtag_info->impcode & EJTAG_V20_IMP_NODB))
			ejtag_info->debug_caps |= EJTAG_DCR_DB;
	} else
		/* keep  debug caps for later use */
		ejtag_info->debug_caps = dcr & (EJTAG_DCR_ENM
				| EJTAG_DCR_IB | EJTAG_DCR_DB);


	if (ejtag_info->debug_caps & EJTAG_DCR_IB) {
		retval = mips32_configure_ibs(target);
		if (retval != ERROR_OK)
			return retval;
	}

	if (ejtag_info->debug_caps & EJTAG_DCR_DB) {
		retval = mips32_configure_dbs(target);
		if (retval != ERROR_OK)
			return retval;
	}

	/* check if target endianness settings matches debug control register */
	if (((ejtag_info->debug_caps & EJTAG_DCR_ENM) && (target->endianness == TARGET_LITTLE_ENDIAN)) ||
		(!(ejtag_info->debug_caps & EJTAG_DCR_ENM) && (target->endianness == TARGET_BIG_ENDIAN))) {
		LOG_WARNING("DCR endianness settings does not match target settings");
		LOG_WARNING("Config file does not match DCR endianness - DCR: 0x%8.8x", ejtag_info->debug_caps);
	}

	LOG_DEBUG("DCR 0x%" PRIx32 " numinst %i numdata %i", dcr, mips32->num_inst_bpoints,
			  mips32->num_data_bpoints);

	mips32->bp_scanned = 1;

	return ERROR_OK;
}

int mips32_enable_interrupts(struct target *target, int enable)
{
	int retval;
	int update = 0;
	uint32_t dcr;

	/* read debug control register */
	retval = target_read_u32(target, EJTAG_DCR, &dcr);
	if (retval != ERROR_OK)
		return retval;

	if (enable) {
		if (!(dcr & EJTAG_DCR_INTE)) {
			/* enable interrupts */
			dcr |= EJTAG_DCR_INTE;
			update = 1;
		}
	} else {
		if (dcr & EJTAG_DCR_INTE) {
			/* disable interrupts */
			dcr &= ~EJTAG_DCR_INTE;
			update = 1;
		}
	}

	if (update) {
		retval = target_write_u32(target, EJTAG_DCR, dcr);
		if (retval != ERROR_OK)
			return retval;
	}

	return ERROR_OK;
}

int mips32_checksum_memory(struct target *target, uint32_t address,
		uint32_t count, uint32_t *checksum)
{
	struct working_area *crc_algorithm;
	struct reg_param reg_params[2];
	struct mips32_algorithm mips32_info;

	/* see contrib/loaders/checksum/mips32.s for src */

	static const uint32_t mips_crc_code[] = {
		0x248C0000,		/* addiu	$t4, $a0, 0 */
		0x24AA0000,		/* addiu	$t2, $a1, 0 */
		0x2404FFFF,		/* addiu	$a0, $zero, 0xffffffff */
		0x10000010,		/* beq		$zero, $zero, ncomp */
		0x240B0000,		/* addiu	$t3, $zero, 0 */
						/* nbyte: */
		0x81850000,		/* lb		$a1, ($t4) */
		0x218C0001,		/* addi		$t4, $t4, 1 */
		0x00052E00,		/* sll		$a1, $a1, 24 */
		0x3C0204C1,		/* lui		$v0, 0x04c1 */
		0x00852026,		/* xor		$a0, $a0, $a1 */
		0x34471DB7,		/* ori		$a3, $v0, 0x1db7 */
		0x00003021,		/* addu		$a2, $zero, $zero */
						/* loop: */
		0x00044040,		/* sll		$t0, $a0, 1 */
		0x24C60001,		/* addiu	$a2, $a2, 1 */
		0x28840000,		/* slti		$a0, $a0, 0 */
		0x01074826,		/* xor		$t1, $t0, $a3 */
		0x0124400B,		/* movn		$t0, $t1, $a0 */
		0x28C30008,		/* slti		$v1, $a2, 8 */
		0x1460FFF9,		/* bne		$v1, $zero, loop */
		0x01002021,		/* addu		$a0, $t0, $zero */
						/* ncomp: */
		0x154BFFF0,		/* bne		$t2, $t3, nbyte */
		0x256B0001,		/* addiu	$t3, $t3, 1 */
		0x7000003F,		/* sdbbp */
	};

	/* make sure we have a working area */
	if (target_alloc_working_area(target, sizeof(mips_crc_code), &crc_algorithm) != ERROR_OK)
		return ERROR_TARGET_RESOURCE_NOT_AVAILABLE;

	/* convert mips crc code into a buffer in target endianness */
	uint8_t mips_crc_code_8[sizeof(mips_crc_code)];
	target_buffer_set_u32_array(target, mips_crc_code_8,
					ARRAY_SIZE(mips_crc_code), mips_crc_code);

	target_write_buffer(target, crc_algorithm->address, sizeof(mips_crc_code), mips_crc_code_8);

	mips32_info.common_magic = MIPS32_COMMON_MAGIC;
	mips32_info.isa_mode = MIPS32_ISA_MIPS32;

	init_reg_param(&reg_params[0], "r4", 32, PARAM_IN_OUT);
	buf_set_u32(reg_params[0].value, 0, 32, address);

	init_reg_param(&reg_params[1], "r5", 32, PARAM_OUT);
	buf_set_u32(reg_params[1].value, 0, 32, count);

	int timeout = 80000 * (1 + (count / (1024 * 1024)));

	int retval = target_run_algorithm(target, 0, NULL, 2, reg_params,
			crc_algorithm->address, crc_algorithm->address + (sizeof(mips_crc_code) - 4), timeout,
			&mips32_info);

	if (retval == ERROR_OK)
		*checksum = buf_get_u32(reg_params[0].value, 0, 32);

	destroy_reg_param(&reg_params[0]);
	destroy_reg_param(&reg_params[1]);

	target_free_working_area(target, crc_algorithm);

	return retval;
}

/** Checks whether a memory region is zeroed. */
int mips32_blank_check_memory(struct target *target,
		uint32_t address, uint32_t count, uint32_t *blank)
{
	struct working_area *erase_check_algorithm;
	struct reg_param reg_params[3];
	struct mips32_algorithm mips32_info;

	static const uint32_t erase_check_code[] = {
						/* nbyte: */
		0x80880000,		/* lb		$t0, ($a0) */
		0x00C83024,		/* and		$a2, $a2, $t0 */
		0x24A5FFFF,		/* addiu	$a1, $a1, -1 */
		0x14A0FFFC,		/* bne		$a1, $zero, nbyte */
		0x24840001,		/* addiu	$a0, $a0, 1 */
		0x7000003F		/* sdbbp */
	};

	/* make sure we have a working area */
	if (target_alloc_working_area(target, sizeof(erase_check_code), &erase_check_algorithm) != ERROR_OK)
		return ERROR_TARGET_RESOURCE_NOT_AVAILABLE;

	/* convert erase check code into a buffer in target endianness */
	uint8_t erase_check_code_8[sizeof(erase_check_code)];
	target_buffer_set_u32_array(target, erase_check_code_8,
					ARRAY_SIZE(erase_check_code), erase_check_code);

	target_write_buffer(target, erase_check_algorithm->address, sizeof(erase_check_code), erase_check_code_8);

	mips32_info.common_magic = MIPS32_COMMON_MAGIC;
	mips32_info.isa_mode = MIPS32_ISA_MIPS32;

	init_reg_param(&reg_params[0], "r4", 32, PARAM_OUT);
	buf_set_u32(reg_params[0].value, 0, 32, address);

	init_reg_param(&reg_params[1], "r5", 32, PARAM_OUT);
	buf_set_u32(reg_params[1].value, 0, 32, count);

	init_reg_param(&reg_params[2], "r6", 32, PARAM_IN_OUT);
	buf_set_u32(reg_params[2].value, 0, 32, 0xff);

	int retval = target_run_algorithm(target, 0, NULL, 3, reg_params,
			erase_check_algorithm->address,
			erase_check_algorithm->address + (sizeof(erase_check_code) - 4),
			10000, &mips32_info);

	if (retval == ERROR_OK)
		*blank = buf_get_u32(reg_params[2].value, 0, 32);

	destroy_reg_param(&reg_params[0]);
	destroy_reg_param(&reg_params[1]);
	destroy_reg_param(&reg_params[2]);

	target_free_working_area(target, erase_check_algorithm);

	return retval;
}

uint32_t DetermineCpuTypeFromPrid(uint32_t prid, uint32_t config, uint32_t config1) {

	uint32_t cpuType;

	/* Determine CPU type from PRID. */
	if (((prid >> 16) & 0xff) == 16)
		/* Altera */
		return (uint32_t)MIPS_MP32;

	if (((prid >> 16) & 0xff) == 2)
		/* Broadcom */
		return (uint32_t) MIPS_BCM;

	if (((prid >> 16) & 0xff) == 3) {
		/* AMD Alchemy processors */
		switch ((prid >> 24) & 0xff)
		{
			case 0x00:
				cpuType = MIPS_AU1000;
				break;

			case 0x01:
				cpuType = MIPS_AU1500;
				break;

			case 0x02:
				cpuType = MIPS_AU1100;
				break;

			case 0x03:
				cpuType = MIPS_AU1550;
				break;

			case 0x04:
				cpuType = MIPS_AU1200;
				break;

			default:
				cpuType = CPUTYPE_UNKNOWN;
				break;
		} /* end of switch */

		return cpuType;
	}

	switch ((prid >> 8) & 0xff)
	{	/* MIPS Technologies cores */
		case 0x80:
			cpuType = MIPS_4Kc;
			break;

		case 0x81:
			if (config1 & 1)
				cpuType = MIPS_5Kf;	   /* fpu present */
			else 
				cpuType = MIPS_5Kc;
			break;

		case 0x82:
			cpuType = MIPS_20Kc;
			break;

		case 0x83:
			if ((config >> 20) & 1)
				cpuType = MIPS_4Kp;
			else
				cpuType = MIPS_4Km;
			break;
					   
		case 0x84:
		case 0x90:
			cpuType = MIPS_4KEc;
			break;

		case 0x85:
		case 0x91:
			if ((config >> 20) & 1) 
				cpuType = MIPS_4KEp;
			else
				cpuType = MIPS_4KEm;
			break;
					   
		case 0x86:
			cpuType = MIPS_4KSc;
			break;

		case 0x87:
			cpuType = MIPS_M4K;
			break;

		case 0x88:
			cpuType = MIPS_25Kf;
			break;

		case 0x89:
			if (config1 & 1)
				cpuType = MIPS_5KEf;	   /* fpu present */
			else
				cpuType = MIPS_5KEc;
			break;

		case 0x92:
			cpuType = MIPS_4KSd;
			break;

		case 0x93:
			if (config1 & 1)
				cpuType = MIPS_24Kf;	   /* fpu present */
			else
				cpuType = MIPS_24Kc;
			break;

		case 0x95:
			if (config1 & 1)
				cpuType = MIPS_34Kf;	   /* fpu present */
			else {
				/* In MT with a single-threaded FPU, Config1.FP may be 0   */
				/* even though an FPU exists.  Scan all TC contexts and if */
				/* any have Config1.FP, then set processor to 100Kf.	   */
				/* skip it for now										   */
				cpuType = MIPS_34Kc;
			}
			break;

		case 0x96:
			if (config1 & 1)
				cpuType = MIPS_24KEf;	   /* fpu present */
			else
				cpuType = MIPS_24KEc;
			break;

		case 0x97:
			if (config1 & 1)
				cpuType = MIPS_74Kf;	   /* fpu present */
			else 
				cpuType = MIPS_74Kc;
			break;

		case 0x99:
			if (config1 & 1) 
				cpuType = MIPS_1004Kf;	   /* fpu present */
			else {
				/* In MT with a single-threaded FPU, Config1.FP may be 0   */
				/* even though an FPU exists.  Scan all TC contexts and if */
				/* any have Config1.FP, then set processor to 100Kf.	   */
				/* skip it for now										   */
				cpuType = MIPS_1004Kc;
			}
			break;

		case 0x9A:
			if (config1 & 1)
				cpuType = MIPS_1074Kf;	   /* fpu present */
			else {
				/* In MT with a single-threaded FPU, Config1.FP may be 0   */
				/* even though an FPU exists.  Scan all TC contexts and if */
				/* any have Config1.FP, then set processor to 100Kf.	   */
				/* skip it for now										   */
				cpuType = MIPS_1074Kc;
			}
			break;

		case 0x9B:
			cpuType = MIPS_M14K;
			break;

		case 0x9C:
			if (config1& 1)
				cpuType = MIPS_M14Kf;	   /* fpu present */
			else
				cpuType = MIPS_M14Kc;
			break;
			   
		case 0x9D:
			if (config1 & 1)
				cpuType = MIPS_M14KEf;
			else
				cpuType = MIPS_M14KE;
			break;

		case 0x9E:
			if (config1 & 1)
				cpuType = MIPS_M14KEcf;
			else 
				cpuType = MIPS_M14KEc;
			break;

		case 0xA0:
			cpuType = MIPS_INTERAPTIV;
			break;

		case 0xA1:
			cpuType = MIPS_INTERAPTIV_CM;
			break;

		case 0xA2:
			cpuType = MIPS_PROAPTIV;
			break;

		case 0xA3:
			cpuType = MIPS_PROAPTIV_CM;
			break;

		case 0xA6:
			cpuType = MIPS_M5100;
			break;

		case 0xA7:
			cpuType = MIPS_M5150;
			break;

		case 0xA8:
			cpuType = MIPS_P5600;
			break;

		case 0xA9:
			cpuType = MIPS_I5500;
			break;

		default:
			cpuType = CPUTYPE_UNKNOWN;
			break;
	} /* end of switch */
	
	return (cpuType);
}

uint32_t DetermineGuestIdWidth(struct mips_ejtag *ejtag_info, uint32_t *width) {
	// if GuestCtl1 is implemented {
	//		save current GuestCtl1 value
	//    write GuestCtl1, placing all-ones in the ID field
	//    read GuestCtl1
	//    in read value, count the consecutive number of 1 bits (starting from bit position 0)
	//		restore GuestCtl1 value	
	// } else width is 0

	uint32_t err = ERROR_OK;
	uint32_t err2 = ERROR_OK;
	uint32_t count = 0;
	bool guestCtl1Clobbered = false;
	uint32_t guestCtl0 = 0;
	uint32_t guestCtl1 = 0;
	uint32_t tempReg = 0;

	if ((err = mips32_pracc_cp0_read(ejtag_info, &guestCtl0, 12, 6)) != ERROR_OK) {
		goto CLEANUP;
	}
	
	if (((guestCtl0 >> 22) & 0x1) != 0) {
		if ((err = mips32_pracc_cp0_read(ejtag_info, &guestCtl1, 10, 4)) != ERROR_OK) {
			goto CLEANUP;
		}

		guestCtl1Clobbered = true;
		tempReg = guestCtl1;
		do {
			tempReg = (((tempReg) & ~0xFF) | 0xFF);
		} while (0);

		if ((err = mips32_pracc_cp0_write(ejtag_info, tempReg, 10, 4)) != ERROR_OK) {
			goto CLEANUP;
		}
		if ((err = mips32_pracc_cp0_read(ejtag_info, &tempReg, 10, 4)) != ERROR_OK) {
			goto CLEANUP;
		}
		while (count < 8 && (tempReg & 0x1) != 0) {
			count++;
			tempReg >>= 1;
		}
	}

CLEANUP:
	if (guestCtl1Clobbered) {
		// restore
		err2 = mips32_pracc_cp0_write(ejtag_info, guestCtl1, 10, 4);
	}
	*width = count;
	return (err != ERROR_OK ? err : err2);
}

static int mips32_verify_pointer(struct command_context *cmd_ctx,
				 struct mips32_common *mips32)
{
	if (mips32->common_magic != MIPS32_COMMON_MAGIC) {
		command_print(cmd_ctx, "target is not an MIPS32");
		return ERROR_TARGET_INVALID;
	}
	return ERROR_OK;
}

COMMAND_HANDLER(handle_mips_semihosting_command)
{
	struct target *target = get_current_target(CMD_CTX);
	struct mips32_common *mips32 = target_to_mips32(target);
	struct mips_ejtag *ejtag_info = &mips32->ejtag_info;

	if (target == NULL) {
		LOG_ERROR("No target selected");
		return ERROR_FAIL;
	}

	if (ejtag_info->ejtag_version < EJTAG_VERSION_51) {
		LOG_ERROR("Ejtag interface does not support FDC");
		return ERROR_FAIL;
	}

	if (mips32->fdc == 0) {
		LOG_ERROR("FDC not supported by this core");
		return ERROR_FAIL;
	}

	if (CMD_ARGC > 0) {
		int semihosting;

		COMMAND_PARSE_ENABLE(CMD_ARGV[0], semihosting);

		if (!target_was_examined(target)) {
			LOG_ERROR("Target not examined yet");
			return ERROR_FAIL;
		}

		if (semihosting) {
			mips32->semihosting = ENABLE_SEMIHOSTING;

		} else {
			mips32->semihosting = DISABLE_SEMIHOSTING;
		}
	}

	command_print(CMD_CTX, "semihosting is %s",
				  mips32->semihosting
				  ? "enabled" : "disabled");

	return ERROR_OK;
}

int mips32_cp0_command(struct command_invocation *cmd)
{
	int retval;

	struct target *target = get_current_target(CMD_CTX);
	struct mips32_common *mips32 = target_to_mips32(target);
	struct mips_ejtag *ejtag_info = &mips32->ejtag_info;

	retval = mips32_verify_pointer(CMD_CTX, mips32);
	if (retval != ERROR_OK)
		return retval;

	if (target->state != TARGET_HALTED) {
		command_print(CMD_CTX, "target must be stopped for \"%s\" command", CMD_NAME);
		return ERROR_OK;
	}

	/* two or more argument, access a single register/select (write if third argument is given) */
	if (CMD_ARGC < 2) {
		uint32_t value;

		if (CMD_ARGC == 0) {
			for (int i = 0; i < MIPS32NUMCP0REGS; i++) {
				if (mips32_cp0_regs[i].core & mips32->cp0_mask) {
					retval = mips32_pracc_cp0_read(ejtag_info, &value, mips32_cp0_regs[i].reg, mips32_cp0_regs[i].sel);
					if (retval != ERROR_OK) {
						command_print(CMD_CTX, "couldn't access reg %s", mips32_cp0_regs[i].name);
						return ERROR_OK;
					}
				} else /* Register name not valid for this core */
					continue;
				
				command_print(CMD_CTX, "%*s: 0x%8.8x", 14, mips32_cp0_regs[i].name, value);
			}
		} else {
			for (int i = 0; i < MIPS32NUMCP0REGS; i++) {
				/* find register name */
				if (mips32_cp0_regs[i].core & mips32->cp0_mask) {
					if (strcmp(mips32_cp0_regs[i].name, CMD_ARGV[0]) == 0) {
						retval = mips32_pracc_cp0_read(ejtag_info, &value, mips32_cp0_regs[i].reg, mips32_cp0_regs[i].sel);
						command_print(CMD_CTX, "0x%8.8x", value);
						return ERROR_OK;
					}
				} else /* Register name not valid for this core */
					continue;
			}

			LOG_ERROR("BUG: register '%s' not found", CMD_ARGV[0]);
			return ERROR_COMMAND_SYNTAX_ERROR;
		}
	} else {
		if (CMD_ARGC == 2) {
			uint32_t value;
			int tmp = *CMD_ARGV[0];

			if (isdigit(tmp) == false) {
				for (int i = 0; i < MIPS32NUMCP0REGS; i++) {
					/* find register name */
					if (mips32_cp0_regs[i].core & mips32->cp0_mask) {
						if (strcmp(mips32_cp0_regs[i].name, CMD_ARGV[0]) == 0) {
							COMMAND_PARSE_NUMBER(u32, CMD_ARGV[1], value);

							/* Check if user is writing to Status register */
							if ((mips32_cp0_regs[i].reg == C0_STATUS) && (mips32_cp0_regs[i].sel == 0)) {
								mips32->core_regs[CACHE_REG_STATUS] = value;
								buf_set_u32(mips32->core_cache->reg_list[CACHE_REG_STATUS].value, 0, 32, value);
								mips32->core_cache->reg_list[CACHE_REG_STATUS].dirty = 1;
							} else /* Cause register ?? Update register cache with new value */
								if ((mips32_cp0_regs[i].reg == C0_CAUSE) && (mips32_cp0_regs[i].sel == 0)) {
									mips32->core_regs[CACHE_REG_CAUSE] = value;
									buf_set_u32(mips32->core_cache->reg_list[CACHE_REG_CAUSE].value, 0, 32, value);
									mips32->core_cache->reg_list[CACHE_REG_CAUSE].dirty = 1;
								} else /* DEPC ? Update cached PC */
									if ((mips32_cp0_regs[i].reg == C0_DEPC) && (mips32_cp0_regs[i].sel == 0)) {
										mips32->core_regs[CACHE_REG_PC] = value;
										buf_set_u32(mips32->core_cache->reg_list[CACHE_REG_PC].value, 0, 32, value);
										mips32->core_cache->reg_list[CACHE_REG_PC].dirty = 1;
									}

							retval = mips32_pracc_cp0_write(ejtag_info, value, mips32_cp0_regs[i].reg, mips32_cp0_regs[i].sel);
							return ERROR_OK;
						}
					} else /* Register name not valid for this core */
						continue;
				}

				LOG_ERROR("BUG: register '%s' not found", CMD_ARGV[0]);
				return ERROR_COMMAND_SYNTAX_ERROR;
			} else {
				uint32_t cp0_reg, cp0_sel;
				COMMAND_PARSE_NUMBER(u32, CMD_ARGV[0], cp0_reg);
				COMMAND_PARSE_NUMBER(u32, CMD_ARGV[1], cp0_sel);

				retval = mips32_pracc_cp0_read(ejtag_info, &value, cp0_reg, cp0_sel);
				if (retval != ERROR_OK) {
					command_print(CMD_CTX,
								  "couldn't access reg %" PRIi32,
								  cp0_reg);
					return ERROR_OK;
				}

				command_print(CMD_CTX, "cp0 reg %" PRIi32 ", select %" PRIi32 ": %8.8" PRIx32,
							  cp0_reg, cp0_sel, value);
			}
		} else if (CMD_ARGC == 3) {
			uint32_t cp0_reg, cp0_sel;
			uint32_t value;

			COMMAND_PARSE_NUMBER(u32, CMD_ARGV[0], cp0_reg);
			COMMAND_PARSE_NUMBER(u32, CMD_ARGV[1], cp0_sel);
			COMMAND_PARSE_NUMBER(u32, CMD_ARGV[2], value);

			/* Check if user is writing to Status register */
			if ((cp0_reg == C0_STATUS) && (cp0_sel == 0)) {
				mips32->core_regs[CACHE_REG_STATUS] = value;
				mips32->core_cache->reg_list[CACHE_REG_STATUS].dirty = 1;
			} else /* Cause register ?? Update register cache with new value */
				if ((cp0_reg == C0_CAUSE) && (cp0_sel == 0)) {
					mips32->core_regs[CACHE_REG_CAUSE] = value;
					mips32->core_cache->reg_list[CACHE_REG_CAUSE].dirty = 1;
				} else /* DEPC ? Update cached PC */
					if ((cp0_reg == C0_DEPC) && (cp0_sel == 0)) {
						mips32->core_regs[CACHE_REG_PC] = value;
						mips32->core_cache->reg_list[CACHE_REG_PC].dirty = 1;
					}

			retval = mips32_pracc_cp0_write(ejtag_info, value, cp0_reg, cp0_sel);
			if (retval != ERROR_OK) {
				command_print(CMD_CTX,
						"couldn't access cp0 reg %" PRIi32 ", select %" PRIi32,
						cp0_reg,  cp0_sel);
				return ERROR_OK;
			}
			command_print(CMD_CTX, "cp0 reg %" PRIi32 ", select %" PRIi32 ": %8.8" PRIx32,
						  cp0_reg, cp0_sel, value);
		}
	}

	return ERROR_OK;
}

int mips32_scan_delay_command(struct command_invocation *cmd)
{
	struct target *target = get_current_target(CMD_CTX);
	struct mips32_common *mips32 = target_to_mips32(target);
	struct mips_ejtag *ejtag_info = &mips32->ejtag_info;

	int retval = mips32_verify_pointer(CMD_CTX, mips32);
	if (retval != ERROR_OK)
		return retval;

	if (CMD_ARGC == 1)
		COMMAND_PARSE_NUMBER(uint, CMD_ARGV[0], ejtag_info->scan_delay);
	else if (CMD_ARGC > 1)
		return ERROR_COMMAND_SYNTAX_ERROR;

	command_print(CMD_CTX, "scan delay: %d nsec", ejtag_info->scan_delay);
	if (ejtag_info->scan_delay >= MIPS32_SCAN_DELAY_LEGACY_MODE) {
		ejtag_info->mode = 0;
		command_print(CMD_CTX, "running in legacy mode");
	} else {
		ejtag_info->mode = 1;
		command_print(CMD_CTX, "running in fast queued mode");
	}

	return ERROR_OK;
}

COMMAND_HANDLER(mips32_handle_cp0_command)
{

	/* Call common code */
	return mips32_cp0_command(cmd);
}

COMMAND_HANDLER(mips32_handle_cpuinfo_command)
{
	int retval;
	struct target *target = get_current_target(CMD_CTX);
	struct mips32_common *mips32 = target_to_mips32(target);
	struct mips_ejtag *ejtag_info = &mips32->ejtag_info;

	CPU_INFO info;

	char text[40]={0};
	uint32_t ways, sets, bpl;

	uint32_t	prid; /* cp0 PRID - 15, 0 */
	uint32_t  config; /*	cp0 config - 16, 0 */
	uint32_t config1; /*	cp0 config - 16, 1 */
	uint32_t config2; /*	cp0 config - 16, 2 */
	uint32_t config3; /*	cp0 config - 16, 3 */
	uint32_t config4; /*	cp0 config - 16, 4 */
	uint32_t config5; /*	cp0 config - 16, 5 */
	uint32_t config7; /*	cp0 config - 16, 7 */

	if (target->state != TARGET_HALTED) {
		command_print(CMD_CTX, "target must be stopped for \"%s\" command", CMD_NAME);
		return ERROR_OK;
	}

	/* No arg.s for now */
	if (CMD_ARGC >= 1)
		return ERROR_COMMAND_SYNTAX_ERROR;

	/* Read PRID and config registers */
	if ((retval = mips32_pracc_cp0_read(ejtag_info, &prid, 15, 0)) != ERROR_OK)
		return retval;

	/* Read Config, Config(1,2,3,5 and 7) registers */
	if ((retval = mips32_pracc_cp0_read(ejtag_info, &config, 16, 0))!= ERROR_OK)
		return retval;

	if ((retval = mips32_pracc_cp0_read(ejtag_info, &config1, 16, 1))!= ERROR_OK)
		return retval;

	if ((retval = mips32_pracc_cp0_read(ejtag_info, &config2, 16, 2))!= ERROR_OK)
		return retval;

	if ((retval = mips32_pracc_cp0_read(ejtag_info, &config3, 16, 3))!= ERROR_OK)
		return retval;

	if ((retval = mips32_pracc_cp0_read(ejtag_info, &config4, 16, 4))!= ERROR_OK)
		return retval;

	if ((retval = mips32_pracc_cp0_read(ejtag_info, &config5, 16, 5))!= ERROR_OK)
		return retval;

	if ((retval = mips32_pracc_cp0_read(ejtag_info, &config7, 16, 7))!= ERROR_OK)
		return retval;

	/* Read and store dspase and mtase (and other) architecture extension presence bits */
	info.dspase = (config3 & 0x00000400) ? 1 : 0;		/* dsp ase */
	info.smase	= (config3 & 0x00000002) ? 1 : 0;		/* smartmips ase */
	info.mtase  = (config3 & 0x00000004) ? 1 : 0;		/* multithreading */
	info.m16ase = (config1 & 0x00000004) ? 1 : 0;		/* mips16(e) ase */
	info.micromipsase = ((config3 >> 14) & 0x3) != 0;
	info.mmutype = (config >> 7) & 7;					/* MMU Type Info */
	info.vzase = (config3 & (1<<23)) ? 1 : 0;			/* VZ */
	LOG_USER("vzase: %d", info.vzase);

	/* Check if Virtualization supported */
		/* TODO List */
	if (info.vzase) {
		/* Core supports Virtualization - now get Guest Info */
		uint32_t width;
		uint32_t guestCtl0;

		if ((retval = mips32_pracc_cp0_read(ejtag_info, &guestCtl0, 12, 6)) != ERROR_OK)
			return retval;

		info.guestCtl1Present = (guestCtl0 >> 22) & 0x1;
		
		if ((retval = DetermineGuestIdWidth(ejtag_info, &width)) != ERROR_OK) {
			return retval;
		}

		info.vzGuestIdWidth = width;
   }

	/* MIPS® SIMD Architecture (MSA) */
	info.msa = (config3 & 0x10000000) ? 1 : 0;
	info.cdmm = (config3 & 0x00000008) ? 1 : 0;
	info.mvh = (config5 & (1<<5)) ? 1 : 0;		/* mvh */

	/* MMU Supported */
	info.tlbEntries = 0;

	/* MMU types */
	if (((info.mmutype == 1) || (info.mmutype == 4)) || ((info.mmutype == 3) && info.vzase)) {
		info.tlbEntries = (((config1 >> 25) & 0x3f)+1);
		info.mmutype = (config >> 7) & 7;
		if (info.mmutype == 1)
			/* VTLB only   !!!Does not account for Config4.ExtVTLB */
			info.tlbEntries = (uint32_t )(((config1 >> 25) & 0x3f)+1);   
		else
			/* root RPU */
			if ((info.mmutype == 3) && info.vzase)
				info.tlbEntries = (uint32_t )(((config1 >> 25) & 0x3f)+1);
			else {
				/*  VTLB and FTLB */
				if (info.mmutype == 4) {
					ways = ftlb_ways[(config4 >> 4) & 0xf];
					sets = ftlb_sets[config4 & 0xf];
					info.tlbEntries = (uint32_t )((((config1 >> 25) & 0x3f)+1) + (ways*sets));
				} else
					info.tlbEntries = 0;
			}
	}

	/* If release 2 of Arch. then get exception base info */
	if (((config >> 10) & 7) != 0) {	/* release 2 */
		uint32_t  ebase;
		if ((retval = mips32_pracc_cp0_read(ejtag_info, &ebase, 15, 1))!= ERROR_OK)
			return retval;

		info.cpuid = (uint32_t)(ebase & 0x1ff);
	} else {
		info.cpuid = 0;
	}

	info.cpuType = DetermineCpuTypeFromPrid(prid, config, config1);

	/* Determine Core info */
	switch (info.cpuType) {
	  case MIPS_4Kc:
		  info.cpuCore = MIPS_4Kc;
		  info.vendor = MIPS_CORE;
		  info.instSet = MIPS32;
		  strcpy(text, "4Kc");
		  break;

	  case MIPS_4Km:
		  info.cpuCore = MIPS_4Km;
		  info.vendor = MIPS_CORE;
		  info.instSet = MIPS32;
		  strcpy(text, "4Km");
		  break;

	  case MIPS_4Kp:
		  info.cpuCore = MIPS_4Kp;
		  info.vendor = MIPS_CORE;
		  info.instSet = MIPS32;
		  strcpy(text, "4Kp");
		  break;

	  case MIPS_4KEc:
		  info.cpuCore = MIPS_4KEc;
		  info.vendor = MIPS_CORE;
		  info.instSet = MIPS32;
		  strcpy(text, "4KEc");		  
		  break;

	  case MIPS_4KEm:
		  info.cpuCore = MIPS_4KEm;
		  info.vendor = MIPS_CORE;
		  info.instSet = MIPS32;
		  strcpy(text, "4KEm");
		  break;
	  case MIPS_4KEp:
		  info.cpuCore = MIPS_4KEp;
		  info.vendor = MIPS_CORE;
		  info.instSet = MIPS32;
		  strcpy(text, "4KEp");
		  break;

	  case MIPS_4KSc:
		  info.cpuCore = MIPS_4KSc;
		  info.vendor = MIPS_CORE;
		  info.instSet = MIPS32;
		  strcpy(text, "4KSc");
		  break;

	  case MIPS_4KSd:
		  info.cpuCore = MIPS_4KSd;
		  info.vendor = MIPS_CORE;
		  info.instSet = MIPS32;
		  strcpy(text, "4KSd");
		  break;

	  case MIPS_M4K:
		  info.cpuCore = MIPS_M4K;
		  info.vendor = MIPS_CORE;
		  info.instSet = MIPS32;
		  strcpy(text, "4K");
		  break;

	  case MIPS_24Kc:
		  info.cpuCore = MIPS_24Kc;
		  info.vendor = MIPS_CORE;
		  info.instSet = MIPS32;
		  strcpy(text, "24Kc");
		  break;

	  case MIPS_24Kf:
		  info.cpuCore = MIPS_24Kf;
		  info.vendor = MIPS_CORE;
		  info.instSet = MIPS32;
		  strcpy(text, "24Kf");
		  break;

	  case MIPS_24KEc:
		  info.cpuCore = MIPS_24KEc;
		  info.vendor = MIPS_CORE;
		  info.instSet = MIPS32;
		  strcpy(text, "24KEc");
		  break;

	  case MIPS_24KEf:
		  info.cpuCore = MIPS_24KEf;
		  info.vendor = MIPS_CORE;
		  info.instSet = MIPS32;
		  strcpy(text, "24KEf");
		  break;

	  case MIPS_34Kc:
		  info.cpuCore = MIPS_34Kc;
		  info.vendor = MIPS_CORE;
		  info.instSet = MIPS32;
		  strcpy(text, "34Kc");
		  break;

	  case MIPS_34Kf:
		  info.cpuCore = MIPS_34Kf;
		  info.vendor = MIPS_CORE;
		  info.instSet = MIPS32;
		  strcpy(text, "3Kf ");
		  break;

	  case MIPS_5Kc:
		  info.cpuCore = MIPS_5Kc;
		  info.vendor = MIPS_CORE;
		  info.instSet = MIPS64;
		  strcpy(text, "5Kc");
		  break;

	  case MIPS_5Kf:
		  info.cpuCore = MIPS_5Kf;
		  info.vendor = MIPS_CORE;
		  info.instSet = MIPS64;
		  strcpy(text, "5Kf");
		  break;

	  case MIPS_5KEc:
		  info.cpuCore = MIPS_5KEc;
		  info.vendor = MIPS_CORE;
		  info.instSet = MIPS64;
		  strcpy(text, "5KEc");
		  break;

	  case MIPS_5KEf:
		  info.cpuCore = MIPS_5KEf;
		  info.vendor = MIPS_CORE;
		  info.instSet = MIPS64;
		  strcpy(text, "5KEf");
		  break;

	  case MIPS_20Kc:
		  info.cpuCore = MIPS_20Kc;
		  info.vendor = MIPS_CORE;
		  info.instSet = MIPS64;
		  strcpy(text, "20Kc");
		  break;

	  case MIPS_25Kf:
		  info.cpuCore = MIPS_25Kf;
		  info.vendor = MIPS_CORE;
		  info.instSet = MIPS64;
		  strcpy(text, "25Kf");
		  break;

	  case MIPS_AU1000:
		  info.cpuCore = MIPS_AU1000;
		  info.vendor = ALCHEMY_CORE;
		  info.instSet = MIPS32;
		  strcpy(text, "AU1000");
		  break;
	  case MIPS_AU1100:
		  info.cpuCore = MIPS_AU1100;
		  info.vendor = ALCHEMY_CORE;
		  info.instSet = MIPS32;
		  strcpy(text, "AU1100");
		  break;

	  case MIPS_AU1200:
		  info.cpuCore = MIPS_AU1200;
		  info.vendor = ALCHEMY_CORE;
		  info.instSet = MIPS32;
		  strcpy(text, "AU1200");
		  break;

	  case MIPS_AU1500:
		  info.cpuCore = MIPS_AU1500;
		  info.vendor = ALCHEMY_CORE;
		  info.instSet = MIPS32;
		  strcpy(text, "AU1500");
		  break;

	  case MIPS_AU1550:
		  info.cpuCore = MIPS_AU1550;
		  info.vendor = ALCHEMY_CORE;
		  info.instSet = MIPS32;
		  strcpy(text, "AU1550");
		  break;

	  case MIPS_74Kc:
		  info.cpuCore = MIPS_74Kc;
		  info.vendor = MIPS_CORE;
		  info.instSet = MIPS32;
		  strcpy(text, "74Kc");
		  break;

	  case MIPS_74Kf:
		  info.cpuCore = MIPS_74Kf;
		  info.vendor = MIPS_CORE;
		  info.instSet = MIPS32;
		  strcpy(text, "74Kf");
		  break;

	  case MIPS_84Kc:
		  info.cpuCore = MIPS_84Kc;
		  info.vendor = MIPS_CORE;
		  info.instSet = MIPS32;
		  strcpy(text, "84Kc");
		  break;

	  case MIPS_84Kf:
		  info.cpuCore = MIPS_84Kf;
		  info.vendor = MIPS_CORE;
		  info.instSet = MIPS32;
		  strcpy(text, "84Kf");
		  break;

	  case MIPS_M14K:
		  info.cpuCore = MIPS_M14K;
		  info.vendor = MIPS_CORE;
		  /*		  if ((err = GetM14KInstSet(handle, &info.instSet)) != SUCCESS) return err; */
		  strcpy(text, "M14K");
		  break;

	  case MIPS_M14Kc:
		  info.cpuCore = MIPS_M14Kc;
		  info.vendor = MIPS_CORE;
		  /*		  if ((err = GetM14KInstSet(handle, &info.instSet)) != SUCCESS) return err; */
		  strcpy(text, "M14Kc");
		  break;

	  case MIPS_M14Kf:
		  info.cpuCore = MIPS_M14Kf;
		  info.vendor = MIPS_CORE;
		  /*		  if ((err = GetM14KInstSet(handle, &info.instSet)) != SUCCESS) return err; */
		  strcpy(text, "M14Kf");
		  break;

	  case MIPS_M14KE:
		  info.cpuCore = MIPS_M14KE;
		  info.vendor = MIPS_CORE;
		  /*		  if ((err = GetM14KInstSet(handle, &info.instSet)) != SUCCESS) return err; */
		  strcpy(text, "microAptiv_UC");
		  break;

	  case MIPS_M14KEf:
		  info.cpuCore = MIPS_M14KEf;
		  info.vendor = MIPS_CORE;
		  /*		  if ((err = GetM14KInstSet(handle, &info.instSet)) != SUCCESS) return err; */
		  strcpy(text, "microAptiv_UCF");
		  break;

	  case MIPS_M14KEc:
		  info.cpuCore = MIPS_M14KEc;
		  info.vendor = MIPS_CORE;
		  /*		  if ((err = GetM14KInstSet(handle, &info.instSet)) != SUCCESS) return err; */
		  strcpy(text, "microAptiv_UP");
		  break;

	  case MIPS_M14KEcf:
		  info.cpuCore = MIPS_M14KEcf;
		  info.vendor = MIPS_CORE;
		  /*		  if ((err = GetM14KInstSet(handle, &info.instSet)) != SUCCESS) return err; */
		  strcpy(text, "microAptiv_UPF");
		  break;

	  case MIPS_M5100:
		  info.cpuCore = MIPS_M5100;
		  info.vendor = MIPS_CORE;
		  /*		  if ((err = GetM14KInstSet(handle, &info.instSet)) != SUCCESS) return err; */
		  strcpy(text, "M5100");
		  break;

	  case MIPS_M5150:
		  info.cpuCore = MIPS_M5150;
		  info.vendor = MIPS_CORE;
		  /*		  if ((err = GetM14KInstSet(handle, &info.instSet)) != SUCCESS) return err; */
		  strcpy(text, "M5150");
		  break;

	  case MIPS_BCM:
		  info.cpuCore = MIPS_BCM;
		  info.vendor = BROADCOM_CORE;
		  info.instSet = MIPS32;
		  strcpy(text, "BCM");
		  break;

	  case MIPS_MP32:
		  info.cpuCore = MIPS_MP32;
		  info.vendor = ALTERA_CORE;
		  info.instSet = MIPS32;
		  strcpy(text, "MP32");
		  break;

	  case MIPS_1004Kc:
		  info.cpuCore = MIPS_1004Kc;
		  info.vendor = MIPS_CORE;
		  info.instSet = MIPS32;
		  strcpy(text, "1004Kc");
		  break;

	  case MIPS_1004Kf:
		  info.cpuCore = MIPS_1004Kf;
		  info.vendor = MIPS_CORE;
		  info.instSet = MIPS32;
		  strcpy(text, "1004Kf");
		  break;

	  case MIPS_1074Kc:
		  info.cpuCore = MIPS_1074Kc;
		  info.vendor = MIPS_CORE;
		  info.instSet = MIPS32;
		  strcpy(text, "1074Kc");
		  break;

	  case MIPS_1074Kf:
		  info.cpuCore = MIPS_1074Kf;
		  info.vendor = MIPS_CORE;
		  info.instSet = MIPS32;
		  strcpy(text, "1074Kf");
		  break;

	  case MIPS_PROAPTIV:
		  info.cpuCore = MIPS_PROAPTIV;
		  info.vendor = MIPS_CORE;
		  info.instSet = MIPS32;
		  strcpy(text, "PROAPTIV");
		  break;

	  case MIPS_PROAPTIV_CM:
		  info.cpuCore = MIPS_PROAPTIV_CM;
		  info.vendor = MIPS_CORE;
		  info.instSet = MIPS32;
		  strcpy(text, "PROAPTIV_CM");
		  break;

	  case MIPS_INTERAPTIV:
		  info.cpuCore = MIPS_INTERAPTIV;
		  info.vendor = MIPS_CORE;
		  info.instSet = MIPS32;
		  strcpy(text, "INTERAPTIV");
		  break;

	  case MIPS_INTERAPTIV_CM:
		  info.cpuCore = MIPS_INTERAPTIV_CM;
		  info.vendor = MIPS_CORE;
		  info.instSet = MIPS32;
		  strcpy(text, "INTERAPTIV_CM");
		  break;

	  case MIPS_P5600:
		  info.cpuCore = MIPS_P5600;
		  info.vendor = MIPS_CORE;
		  info.instSet = MIPS32;
		  strcpy(text, "P5600");
		  break;

	  case MIPS_I5500:
		  info.cpuCore = MIPS_I5500;
		  info.vendor = MIPS_CORE;
		  info.instSet = MIPS32;
		  strcpy(text, "I5500");
		  break;
	}

	/* Determine Instr Cache Size */
	ways = wayTable[(config1 >> CFG1_IASHIFT) & 7];
	sets = setTableISDS[(config1 >> CFG1_ISSHIFT) & 7];
	bpl  = bplTable[(config1 >> CFG1_ILSHIFT) & 7];
	info.iCacheSize = ways*sets*bpl;

	/* Determine data cache size */
	ways = wayTable[(config1 >>  CFG1_DASHIFT) & 7];
	sets = setTableISDS[(config1 >> CFG1_DSSHIFT) & 7];
	bpl  = bplTable[(config1 >> CFG1_DLSHIFT) & 7];
	info.dCacheSize = ways*sets*bpl;

	/* Display Core Type info */
	LOG_USER ("cpuCore: MIPS_%s", &text[0]);

	LOG_USER ("cputype: %d", info.cpuType);

	/* Display Core Vendor ID */
	switch (info.vendor) {
		case MIPS_CORE:
			strcpy(text, "MIPS");
			break;

		case ALCHEMY_CORE:
			strcpy(text, "Alchemy");
			break;

		case BROADCOM_CORE:
			strcpy(text, "Broadcom");
			break;

		case ALTERA_CORE:
			strcpy(text, "Altera");
			break;

		default:
			sprintf (text, "Unknown CPU vendor code %u.", ((prid & 0x00ffff00) >> 16));
			break;
	}

	/* Display Core Vendor */
	LOG_USER (" vendor: %s", &text[0]);
	LOG_USER ("  cpuid: %d", info.cpuid);
	switch ((((config3 & 0x0000C000) >>  14))){
		case 0:
			strcpy (text, "MIPS32");
			break;
		case 1:
			strcpy (text, "microMIPS");
			break;
		case 2:
			strcpy (text, "MIPS32 (at reset) and microMIPS");
			break;

		case 3:
			strcpy (text, "microMIPS (at reset) and MIPS32");
			break;
	}

	/* Display Instruction Set Info */
	LOG_USER ("instr Set: %s", &text[0]);
	LOG_USER ("prid: %x", prid);
	uint32_t rev = prid & 0x000000ff;
	LOG_USER ("rtl: %x.%x.%x", (rev & 0xE0), (rev & 0x1C), (rev & 0x3));

	LOG_USER ("Instr Cache: %d", info.iCacheSize);
	LOG_USER (" Data Cache: %d", info.dCacheSize);

	LOG_USER ("Max Number of Instr Breakpoints: %d", mips32->num_inst_bpoints);
	LOG_USER ("Max Number of  Data Breakpoints: %d", mips32->num_data_bpoints);

	if (info.mtase){
		LOG_USER("mta: true");

		/* Get VPE and Thread info */
		uint32_t tcbind;
		uint32_t mvpconf0;

		/* Read tcbind register */
		if ((retval = mips32_pracc_cp0_read(ejtag_info, &tcbind, 2, 2))!= ERROR_OK)
			return retval;

		LOG_USER("curvpe: %d", (tcbind & 0xf));
		LOG_USER(" curtc: %d", ((tcbind >> 21) & 0xff));

		/* Read mvpconf0 register */
		if ((retval = mips32_pracc_cp0_read(ejtag_info, &mvpconf0, 0, 2))!= ERROR_OK)
			return retval;

		LOG_USER(" numtc: %d", (mvpconf0 & 0xf)+1);
		LOG_USER("numvpe: %d", ((mvpconf0 >> 10) & 0xf)+1);
	}
	else {
		LOG_USER("mta: false");
	}

	switch (info.mmutype) {
		case MMU_TLB:
			strcpy (text, "TLB");
			break;
		case MMU_BAT:
			strcpy (text, "BAT");
			break;
		case MMU_FIXED:
			strcpy (text, "FIXED");
			break;
		case MMU_DUAL_VTLB_FTLB:
			strcpy (text, "DUAL VAR/FIXED");
			break;
		default:
			strcpy (text, "Unknown");
	}

	LOG_USER("MMU Type: %s", &text[0]);
	LOG_USER("TLB Enties: %d", info.tlbEntries);

	/* does the core support a DSP */
	if (info.dspase)
		strcpy(text, "true");
	else
		strcpy(text, "false");

	LOG_USER ("dsp: %s", &text[0]);

	if (info.smase)
		strcpy(text, "true");
	else
		strcpy(text, "false");

	LOG_USER ("Smart Mips ASE: %s", &text[0]);

	/* MIPS® SIMD Architecture (MSA) */
	if (info.msa)
		strcpy(text, "true");
	else
		strcpy(text, "false");

	LOG_USER ("msa: %s", &text[0]);

	/*Move To/From High COP0 (MTHC0/MFHC0) instructions are implemented. */
	if (info.mvh)
		strcpy(text, "true");
	else
		strcpy(text, "false");

	LOG_USER ("mvh: %s", &text[0]);

	/* Common Device Memory Map implemented? */
	if (info.cdmm)
		strcpy(text, "true");
	else
		strcpy(text, "false");

	LOG_USER ("cdmm: %s", &text[0]);

	return ERROR_OK;
}

COMMAND_HANDLER(mips32_handle_dsp_command)
{
	int retval;
	struct target *target = get_current_target(CMD_CTX);
	struct mips32_common *mips32 = target_to_mips32(target);
	struct mips_ejtag *ejtag_info = &mips32->ejtag_info;

	retval = mips32_verify_pointer(CMD_CTX, mips32);
	if (retval != ERROR_OK)
		return retval;

	if (target->state != TARGET_HALTED) {
		command_print(CMD_CTX, "target must be stopped for \"%s\" command", CMD_NAME);
		return ERROR_OK;
	}

	/* Check for too many command arg.s */
	if (CMD_ARGC >= 3) 
		return ERROR_COMMAND_SYNTAX_ERROR;

	/* Check if DSP access supported or not */
	if (mips32->dsp_implemented == DSP_NOT_IMP) {

		/* Issue Error Message */
		command_print(CMD_CTX, "DSP not implemented by this processor");
		return ERROR_OK;
	}

//	if (mips32->dsp_rev == DSP_REV1) {
//		command_print(CMD_CTX, "DSP Rev 1 not supported");
//		return ERROR_OK;
//	}

	/* two or more argument, access a single register/select (write if third argument is given) */
	if (CMD_ARGC < 2) {
		uint32_t value;

		if (CMD_ARGC == 0) {
			value = 0;
			for (int i = 0; i < MIPS32NUMDSPREGS; i++) {
				retval = mips32_pracc_read_dsp_regs(ejtag_info, &value, i);
				if (retval != ERROR_OK) {
					command_print(CMD_CTX, "couldn't access reg %s", mips32_dsp_regs[i].name);
					return retval;
				}
				command_print(CMD_CTX, "%*s: 0x%8.8x", 7, mips32_dsp_regs[i].name, value);
			}
		} else {
			for (int i = 0; i < MIPS32NUMDSPREGS; i++) {
				/* find register name */
				if (strcmp(mips32_dsp_regs[i].name, CMD_ARGV[0]) == 0) {
					retval = mips32_pracc_read_dsp_regs(ejtag_info, &value, i);
					command_print(CMD_CTX, "0x%8.8x", value);
					return retval;
				}
			}

			LOG_ERROR("BUG: register '%s' not found", CMD_ARGV[0]);
			return ERROR_COMMAND_SYNTAX_ERROR;
		}
	} else {
		if (CMD_ARGC == 2) {
			uint32_t value;
			int tmp = *CMD_ARGV[0];

			if (isdigit(tmp) == false) {
				for (int i = 0; i < MIPS32NUMDSPREGS; i++) {
					/* find register name */
					if (strcmp(mips32_dsp_regs[i].name, CMD_ARGV[0]) == 0) {
						COMMAND_PARSE_NUMBER(u32, CMD_ARGV[1], value);
						retval = mips32_pracc_write_dsp_regs(ejtag_info, value, i);
						return retval;
					}
				}

				LOG_ERROR("BUG: register '%s' not found", CMD_ARGV[0]);
				return ERROR_COMMAND_SYNTAX_ERROR;
			}
		}
	}

	return ERROR_OK;
}

COMMAND_HANDLER(mips32_handle_invalidate_cache_command)
{
	int retval = -1;
	struct target *target = get_current_target(CMD_CTX);
	struct mips32_common *mips32 = target_to_mips32(target);
	struct mips_ejtag *ejtag_info = &mips32->ejtag_info;
	int i = 0;

	char *cache_msg[] = {"all", "instr", "data", "L23", NULL, NULL, NULL};

	if (target->state != TARGET_HALTED) {
		LOG_WARNING("target not halted");
		return ERROR_TARGET_NOT_HALTED;
	}

	if ((CMD_ARGC >= 2) || (CMD_ARGC == 0)){
		LOG_DEBUG("ERROR_COMMAND_SYNTAX_ERROR");
		return ERROR_COMMAND_SYNTAX_ERROR;
	}

	if (CMD_ARGC == 1) {
		/* PARSE command options - all/inst/data/allnowb/datanowb */
		for (i = 0; i < 5 ; i++) {
			if (strcmp(CMD_ARGV[0], invalidate_cmd[i].arg) == 0) {
				switch (invalidate_cmd[i].option) {
					case ALL:
						LOG_INFO("clearing %s cache", cache_msg[1]);
						/* For this case - ignore any errors checks, just in case core has no instruction cache */
						mips32_pracc_invalidate_cache(target, ejtag_info, invalidate_cmd[1].option);

						/* TODO: Add L2 code */
						/* LOG_INFO("clearing %s cache", cache_msg[3]); */
						/* retval = mips32_pracc_invalidate_cache(target, ejtag_info, L2); */
						/* if (retval != ERROR_OK) */
						/*	return retval; */

						LOG_INFO("clearing %s cache", cache_msg[2]);
						/* For this case - ignore any errors checks, just in case core has no data cache */
						mips32_pracc_invalidate_cache(target, ejtag_info, invalidate_cmd[2].option);

						break;

					case INST:
						LOG_INFO("clearing %s cache", cache_msg[invalidate_cmd[i].option]);
						retval = mips32_pracc_invalidate_cache(target, ejtag_info, invalidate_cmd[i].option);
						if (retval != ERROR_OK)
							return retval;

						break;

					case DATA:
						LOG_INFO("clearing %s cache", cache_msg[invalidate_cmd[i].option]);
						retval = mips32_pracc_invalidate_cache(target, ejtag_info, invalidate_cmd[i].option);
						if (retval != ERROR_OK)
							return retval;

						break;

					case ALLNOWB:
						LOG_INFO("invalidating %s cache", cache_msg[invalidate_cmd[1].option]);
						retval = mips32_pracc_invalidate_cache(target, ejtag_info, invalidate_cmd[1].option);
						if (retval != ERROR_OK)
							return retval;

						/* TODO: Add L2 code */
						/* LOG_INFO("invalidating %s cache no writeback", cache_msg[3]); */
						/* retval = mips32_pracc_invalidate_cache(target, ejtag_info, L2); */
						/* if (retval != ERROR_OK) */
						/*	return retval; */

						LOG_INFO("invalidating %s cache - no writeback", cache_msg[2]);
						retval = mips32_pracc_invalidate_cache(target, ejtag_info, invalidate_cmd[2].option);
						if (retval != ERROR_OK)
							return retval;

						break;

					case DATANOWB:
						LOG_INFO("invalidating %s cache - no writeback", cache_msg[2]);
						retval = mips32_pracc_invalidate_cache(target, ejtag_info, invalidate_cmd[i].option);
						if (retval != ERROR_OK)
							return retval;
						break;

					default:
						LOG_ERROR("Invalid command argument '%s' not found", CMD_ARGV[0]);
						return ERROR_COMMAND_SYNTAX_ERROR;
						break;
				}

				if (retval == ERROR_FAIL)
					return ERROR_FAIL;
				else
					break;
			} else {
				if (i >= DATANOWB) {
					LOG_ERROR("Invalid command argument '%s' not found", CMD_ARGV[0]);
					return ERROR_COMMAND_SYNTAX_ERROR;
				}
			}

		}
	}
//else {
//		/* default is All */
//		LOG_INFO("invalidating %s cache", cache_msg[1]);
//		retval = mips32_pracc_invalidate_cache(target, ejtag_info, invalidate_cmd[1].option);
//		if (retval != ERROR_OK)
//			return retval;

		/* TODO: Add L2 code */
		/* LOG_INFO("invalidating %s cache", cache_msg[3]); */
		/* retval = mips32_pracc_invalidate_cache(target, ejtag_info, L2); */
		/* if (retval != ERROR_OK) */
		/*	return retval; */

//		LOG_INFO("invalidating %s cache", cache_msg[2]);
//		retval = mips32_pracc_invalidate_cache(target, ejtag_info, invalidate_cmd[2].option);
//		if (retval != ERROR_OK)
//			return retval;
//	}

	return ERROR_OK;
}

COMMAND_HANDLER(mips32_handle_dump_tlb_command)
{
	struct target *target = get_current_target(CMD_CTX);
	struct mips32_common *mips32 = target_to_mips32(target);
	struct mips_ejtag *ejtag_info = &mips32->ejtag_info;
	int retval;

	uint32_t data[4];

	if (target->state != TARGET_HALTED) {
		LOG_WARNING("target not halted");
		return ERROR_TARGET_NOT_HALTED;
	}

	if (CMD_ARGC >= 2){
		LOG_DEBUG("ERROR_COMMAND_SYNTAX_ERROR");
		return ERROR_COMMAND_SYNTAX_ERROR;
	}

	uint32_t config;	/* cp0 config - 16, 0 */
	uint32_t config1;	/* cp0 config1 - 16, 1 */

	/* Read Config register */
	if ((retval = mips32_pracc_cp0_read(ejtag_info, &config, 16, 0))!= ERROR_OK)
		return retval;

	/* Read Config1 register */
	if ((retval = mips32_pracc_cp0_read(ejtag_info, &config1, 16, 1))!= ERROR_OK)
		return retval;
	else {
		uint32_t mmutype;
		uint32_t tlbEntries;

		mmutype = (config >> 7) & 7;
		if (mmutype == 0) {
			LOG_USER("mmutype: %d, No TLB configured", mmutype);
			return ERROR_OK;
		}

		if ((CMD_ARGC == 0) || (CMD_ARGC == 1)){
			uint32_t i = 0;

			/* Get number of TLB entries */
			if (CMD_ARGC == 1) {
				COMMAND_PARSE_NUMBER(u32, CMD_ARGV[0], i);
				tlbEntries = (((config1 >> 25) & 0x3f)+1);
				if (i >= tlbEntries) {
					LOG_USER ("Invalid TLB entry specified - Valid entry #'s are 0-%d", tlbEntries-1);
					return ERROR_COMMAND_SYNTAX_ERROR;
				}

				tlbEntries = i+1;
			} else
				tlbEntries = (((config1 >> 25) & 0x3f)+1);

			LOG_USER("index\t entrylo0\t entrylo1\t  entryhi\t pagemask");
			for (; i < tlbEntries; i++) {
				mips32_pracc_read_tlb_entry(ejtag_info, &data[0], i);
				command_print(CMD_CTX, "  %d\t0x%8.8x\t0x%8.8x\t0x%8.8x\t0x%8.8x", i, data[0], data[1], data[2], data[3]);
			}

		}
	}

	return ERROR_OK;
}

extern void ejtag_main_print_imp(struct mips_ejtag *ejtag_info);
extern int mips_ejtag_get_impcode(struct mips_ejtag *ejtag_info, uint32_t *impcode);
COMMAND_HANDLER(mips32_handle_ejtag_reg_command)
{
	struct target *target = get_current_target(CMD_CTX);
	struct mips32_common *mips32 = target_to_mips32(target);
	struct mips_ejtag *ejtag_info = &mips32->ejtag_info;

	uint32_t idcode;
	uint32_t impcode;
	uint32_t ejtag_ctrl;
	uint32_t dcr;
	int retval;

	retval = mips_ejtag_get_idcode(ejtag_info, &idcode);
	retval = mips_ejtag_get_impcode (ejtag_info, &impcode);
	mips_ejtag_set_instr(ejtag_info, EJTAG_INST_CONTROL);
	ejtag_ctrl = ejtag_info->ejtag_ctrl;
	retval = mips_ejtag_drscan_32(ejtag_info, &ejtag_ctrl);

	if (retval != ERROR_OK)
		LOG_INFO("Encounter an Error");

	LOG_USER ("       idcode: 0x%8.8x", idcode);
	LOG_USER ("      impcode: 0x%8.8x", impcode);
	LOG_USER ("ejtag control: 0x%8.8x", ejtag_ctrl);

	ejtag_main_print_imp(ejtag_info);

	/* Display current DCR */
	retval = target_read_u32(target, EJTAG_DCR, &dcr);
	LOG_USER("          DCR: 0x%8.8x", dcr);

	if (((dcr > 22) & 0x1) == 1)
		LOG_USER("DAS supported");

	if (((dcr > 18) & 0x1) == 1)
		LOG_USER("FDC supported");

	if (((dcr > 17) & 0x1) == 1)
		LOG_USER("DataBrk supported");

	if (((dcr > 16) & 0x1) == 1)
		LOG_USER("InstBrk supported");

	if (((dcr > 15) & 0x1) == 1)
		LOG_USER("Inverted Data value supported");

	if (((dcr > 14) & 0x1) == 1)
		LOG_USER("Data value stored supported");

	if (((dcr > 10) & 0x1) == 1)
		LOG_USER("Complex Breakpoints supported");

	if (((dcr > 9) & 0x1) == 1)
		LOG_USER("PC Sampling supported");

	return ERROR_OK;
}

COMMAND_HANDLER(mips32_handle_scan_delay_command)
{
	return mips32_scan_delay_command(cmd);
}

static const struct command_registration mips32_exec_command_handlers[] = {
	{
		.name = "cp0",
		.handler = mips32_handle_cp0_command,
		.mode = COMMAND_EXEC,
		.help = "display/modify cp0 register(s)",
		.usage = "[[reg_name|regnum select] [value]]",
	},
	{
		.name = "cpuinfo",
		.handler = mips32_handle_cpuinfo_command,
		.mode = COMMAND_EXEC,
		.help = "cpuinfo displays information for the current CPU core.",
		.usage = "cpuinfo",
	},
	{
		.name = "dsp",
		.handler = mips32_handle_dsp_command,
		.mode = COMMAND_EXEC,
		.help = "display or set DSP register; "
		"with no arguments, displays all registers and their values",
		.usage = "[register_name] [value]]",
	},
	{
		.name = "invalidate",
		.handler = mips32_handle_invalidate_cache_command,
		.mode = COMMAND_EXEC,
		.help = "Invalidate either or both the instruction and data caches.",
		.usage = "all|inst|data|allnowb|datanowb",
	},
	{
		.name = "scan_delay",
		.handler = mips32_handle_scan_delay_command,
		.mode = COMMAND_ANY,
		.help = "display/set scan delay in nano seconds",
		.usage = "[value]",
	},
	{
		.name = "dump_tlb",
		.handler = mips32_handle_dump_tlb_command,
		.mode = COMMAND_ANY,
		.help = "dump_tlb",
		.usage = "[entry]",
	},
	{
		"semihosting",
		.handler = handle_mips_semihosting_command,
		.mode = COMMAND_EXEC,
		.usage = "['enable'|'disable']",
		.help = "activate support for semihosting operations",
	},
	{
		.name = "ejtag_reg",
		.handler = mips32_handle_ejtag_reg_command,
		.mode = COMMAND_ANY,
		.help = "read ejtag registers",
		.usage = "",
	},
	COMMAND_REGISTRATION_DONE
};

const struct command_registration mips32_command_handlers[] = {
	{
		.name = "mips32",
		.mode = COMMAND_ANY,
		.help = "mips32 command group",
		.usage = "",
		.chain = mips32_exec_command_handlers,
	},
	COMMAND_REGISTRATION_DONE
};
