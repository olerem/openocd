/***************************************************************************
 *   Copyright (C) 2008 by Spencer Oliver                                  *
 *   spen@spen-soft.co.uk                                                  *
 *                                                                         *
 *   Copyright (C) 2008 by David T.L. Wong                                 *
 *                                                                         *
 *   Copyright (C) 2009 by David N. Claffey <dnclaffey@gmail.com>          *
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "breakpoints.h"
#include "mips32.h"
#include "mips_m4k.h"
#include "mips_mAptiv.h"
#include "mips_common.h"
#include "mips32_dmaacc.h"
#include "target_type.h"
#include "register.h"

static void mips_mAptiv_enable_breakpoints(struct target *target);
static void mips_mAptiv_enable_watchpoints(struct target *target);
static int mips_mAptiv_set_breakpoint(struct target *target,
		struct breakpoint *breakpoint);
static int mips_mAptiv_unset_breakpoint(struct target *target,
		struct breakpoint *breakpoint);
static int mips_mAptiv_internal_restore(struct target *target, int current,
		uint32_t address, int handle_breakpoints,
		int debug_execution);
static int mips_mAptiv_halt(struct target *target);
static int mips_mAptiv_bulk_write_memory(struct target *target, uint32_t address,
		uint32_t count, const uint8_t *buffer);

static int mips_mAptiv_set_breakpoint(struct target *target,
		struct breakpoint *breakpoint)
{
	struct mips32_common *mips32 = target_to_mips32(target);
	struct mips_ejtag *ejtag_info = &mips32->ejtag_info;
	struct mips32_comparator *comparator_list = mips32->inst_break_list;
	int retval;
//	int cur_pc = mips32->core_cache->reg_list[MIPS32_PC].value;

	if (breakpoint->set) {
		LOG_WARNING("breakpoint already set");
		return ERROR_OK;
	}

	if (breakpoint->type == BKPT_HARD) {
		int bp_num = 0;

		while (comparator_list[bp_num].used && (bp_num < mips32->num_inst_bpoints))
			bp_num++;

		if (bp_num >= mips32->num_inst_bpoints) {
			LOG_ERROR("Can not find free FP Comparator(bpid: %" PRIu32 ")",
					breakpoint->unique_id);
			return ERROR_TARGET_RESOURCE_NOT_AVAILABLE;
		}

		breakpoint->set = bp_num + 1;
		comparator_list[bp_num].used = 1;

		/* EJTAG 2.0 uses 30bit IBA. First 2 bits are reserved.
		 * Warning: there is no IB ASID registers in 2.0.
		 * Do not set it! :) */
		if (ejtag_info->ejtag_version == EJTAG_VERSION_20)
			comparator_list[bp_num].bp_value &= 0xFFFFFFFC;

		/* Check for microMips and executing in MIPS16 ISA */
		if (mips32->mmips != MIPS32_ONLY) {
			if ((breakpoint->length == 3) || (breakpoint->length == 5)){
				comparator_list[bp_num].bp_value = breakpoint->address | 1;
				target_write_u32(target, comparator_list[bp_num].reg_address,
								 comparator_list[bp_num].bp_value);
			}
			else {
				comparator_list[bp_num].bp_value = breakpoint->address;
				target_write_u32(target, comparator_list[bp_num].reg_address,
								 comparator_list[bp_num].bp_value);
			}
		}
		else {
			comparator_list[bp_num].bp_value = breakpoint->address;
				target_write_u32(target, comparator_list[bp_num].reg_address,
								 comparator_list[bp_num].bp_value);
		}
		
		comparator_list[bp_num].bp_value = breakpoint->address;

		target_write_u32(target, comparator_list[bp_num].reg_address +
				 ejtag_info->ejtag_ibm_offs, 0x00000000);

		target_write_u32(target, comparator_list[bp_num].reg_address +
				 ejtag_info->ejtag_ibc_offs, 1);
		LOG_DEBUG("bpid: %" PRIu32 ", bp_num %i bp_value 0x%" PRIx32 "",
				  breakpoint->unique_id,
				  bp_num, comparator_list[bp_num].bp_value);

	} else if (breakpoint->type == BKPT_SOFT) {
		LOG_DEBUG("bpid: %" PRIu32, breakpoint->unique_id);

		/* IF GDB sends bp->length 5 for microMips support then change it to 4 */
		/* Check if kind field, is indicated microMips Break */
		/* Verify address is aligned 4 byte boundary and replacing a 32-bit instruction */
		if ((breakpoint->length == 4) || ((breakpoint->length == 5) && ((breakpoint->address % 4) == 0))) {
			uint32_t verify = 0xffffffff;
			uint32_t breakpt_instr;

			/* Remove isa_mode info from length to adjust to correct instruction size */
			if (breakpoint->length == 5) {
				breakpt_instr = MICRO_MIPS32_SDBBP;
			}
			else
				breakpt_instr = MIPS32_SDBBP;

			retval = target_read_memory(target, breakpoint->address, (breakpoint->length & 0xE), 1, breakpoint->orig_instr);
			if (retval != ERROR_OK){
				LOG_DEBUG("target_read_memory failed");
				return retval;
			}

			retval = target_write_u32(target, breakpoint->address, breakpt_instr);
			if (retval != ERROR_OK){
				LOG_DEBUG("target_write_u32 failed");
				return retval;
			}

			retval = target_read_u32(target, breakpoint->address, &verify);
			if (retval != ERROR_OK){
				LOG_DEBUG("target_read_u32 failed");
				return retval;
			}

			if ((breakpt_instr == MIPS32_SDBBP) && (verify != MIPS32_SDBBP)){
				LOG_ERROR("Unable to set 32bit breakpoint at address %08" PRIx32
						  " - check that memory is read/writable", breakpoint->address);
				return ERROR_OK;
			}
			else {
				if ((breakpt_instr == MICRO_MIPS32_SDBBP) && (verify != MICRO_MIPS32_SDBBP)){
						LOG_ERROR("Unable to set microMips32 breakpoint at address %08" PRIx32
								  " - check that memory is read/writable", breakpoint->address);
						return ERROR_OK;
				}
			}

		} else {
			uint16_t verify = 0xffff;
			uint16_t breakpt_instr;

			/* IF GDB sends bp->length 3 for microMips support then change it to 2 */
			/* Check if kind field, is indicated microMips Break */
			if ((breakpoint->length == 3) || ((breakpoint->length == 5) && ((breakpoint->address % 4) != 0))){
				breakpoint->length = 2;
				breakpt_instr = MICRO_MIPS_SDBBP;
			}
			else {
				breakpt_instr = MIPS16_SDBBP;
			}

			retval = target_read_memory(target, breakpoint->address, 2, 1, breakpoint->orig_instr);
			if (retval != ERROR_OK){
				LOG_DEBUG("target_read_memory failed");
				return retval;
			}

			retval = target_write_u16(target, breakpoint->address, breakpt_instr);
			if (retval != ERROR_OK){
				LOG_DEBUG("target_write_u16 failed");
				return retval;
			}

			retval = target_read_u16(target, breakpoint->address, &verify);
			if (retval != ERROR_OK){
				LOG_DEBUG("target_read_u16 failed");
				return retval;
			}

			if ((breakpt_instr == MIPS16_SDBBP) && (verify != MIPS16_SDBBP)){
					LOG_ERROR("Unable to set 16bit breakpoint at address %08" PRIx32
							  " - check that memory is read/writable", breakpoint->address);
					return ERROR_OK;
			}
			else {
				if ((breakpt_instr == MICRO_MIPS_SDBBP) && (verify != MICRO_MIPS_SDBBP)){
						LOG_ERROR("Unable to set microMips breakpoint at address %08" PRIx32
								  " - check that memory is read/writable", breakpoint->address);
						return ERROR_OK;
				}
			}
		}

		breakpoint->set = 20; /* Any nice value but 0 */
	}

	return ERROR_OK;
}

static int mips_mAptiv_unset_breakpoint(struct target *target,
		struct breakpoint *breakpoint)
{
	/* get pointers to arch-specific information */
	struct mips32_common *mips32 = target_to_mips32(target);
	struct mips_ejtag *ejtag_info = &mips32->ejtag_info;
	struct mips32_comparator *comparator_list = mips32->inst_break_list;
	int retval;

	if (!breakpoint->set) {
		LOG_WARNING("breakpoint not set");
		return ERROR_OK;
	}

	if (breakpoint->type == BKPT_HARD) {
		int bp_num = breakpoint->set - 1;
		if ((bp_num < 0) || (bp_num >= mips32->num_inst_bpoints)) {
			LOG_DEBUG("Invalid FP Comparator number in breakpoint (bpid: %" PRIu32 ")",
					  breakpoint->unique_id);
			return ERROR_OK;
		}

		LOG_INFO("bpid: %" PRIu32 " - releasing hw: %d",
				breakpoint->unique_id,
				bp_num);

		comparator_list[bp_num].used = 0;
		comparator_list[bp_num].bp_value = 0;

		target_write_u32(target, comparator_list[bp_num].reg_address +
				 ejtag_info->ejtag_ibc_offs, 0);
	} else {
		/* restore original instruction (kept in target endianness) */
		LOG_DEBUG("bpid: %" PRIu32, breakpoint->unique_id);
		if ((breakpoint->length == 4) || ((breakpoint->length == 5) && ((breakpoint->address % 4) == 0))) {
			uint32_t current_instr;

			/* check that user program has not modified breakpoint instruction */
			/* Remove isa_mode info from length of read */
			retval = target_read_memory(target, breakpoint->address, (breakpoint->length & 0xE), 1, (uint8_t *)&current_instr);
			if (retval != ERROR_OK){
				LOG_DEBUG("target_read_memory failed");
				return retval;
			}

			/**
			 * target_read_memory() gets us data in _target_ endianess.
			 * If we want to use this data on the host for comparisons with some macros
			 * we must first transform it to _host_ endianess using target_buffer_get_u32().
			 */
			current_instr = target_buffer_get_u32(target, (uint8_t *)&current_instr);

			if ((current_instr == MIPS32_SDBBP) || (current_instr == MICRO_MIPS32_SDBBP)){
				retval = target_write_memory(target, breakpoint->address, 4, 1, breakpoint->orig_instr);
				if (retval != ERROR_OK)
					return retval;
			}
			else {
				LOG_WARNING("memory modified: no SDBBP instruction found");
				LOG_WARNING("orignal instruction not written back to memory");
			}

		} else {
			uint16_t current_instr;

			/* check that user program has not modified breakpoint instruction */
			retval = target_read_memory(target, breakpoint->address, 2, 1, (uint8_t *)&current_instr);
			if (retval != ERROR_OK){
				LOG_DEBUG("target_read_memory failed");
				return retval;
			}

			current_instr = target_buffer_get_u16(target, (uint8_t *)&current_instr);
			if ((current_instr == MIPS16_SDBBP) || (current_instr == MICRO_MIPS_SDBBP)){
				retval = target_write_memory(target, breakpoint->address, 2, 1, breakpoint->orig_instr);
				if (retval != ERROR_OK){
					LOG_DEBUG("target_write_memory failed");
					return retval;
				}
			}
			else {
				LOG_WARNING("memory modified: no SDBBP instruction found");
				LOG_WARNING("orignal instruction not written back to memory");
			}
		}
	}
	breakpoint->set = 0;

	return ERROR_OK;
}

static int mips_mAptiv_add_breakpoint(struct target *target, struct breakpoint *breakpoint)
{
	struct mips32_common *mips32 = target_to_mips32(target);

	if (breakpoint->type == BKPT_HARD) {
		if (mips32->num_inst_bpoints_avail < 1) {
			LOG_INFO("no hardware breakpoint available");
			return ERROR_TARGET_RESOURCE_NOT_AVAILABLE;
		}

		mips32->num_inst_bpoints_avail--;
	}

	return mips_mAptiv_set_breakpoint(target, breakpoint);
}

static int mips_mAptiv_remove_breakpoint(struct target *target,
		struct breakpoint *breakpoint)
{
	/* get pointers to arch-specific information */
	struct mips32_common *mips32 = target_to_mips32(target);

	if (target->state != TARGET_HALTED) {
		LOG_WARNING("target not halted");
		return ERROR_TARGET_NOT_HALTED;
	}

	if (breakpoint->set)
		mips_mAptiv_unset_breakpoint(target, breakpoint);

	if (breakpoint->type == BKPT_HARD)
		mips32->num_inst_bpoints_avail++;

	return ERROR_OK;
}


static int mips_mAptiv_init_target(struct command_context *cmd_ctx,
		struct target *target)
{
	mips32_build_reg_cache(target);

	return ERROR_OK;
}

static int mips_mAptiv_init_arch_info(struct target *target,
		struct mips_mAptiv_common *mips_mAptiv, struct jtag_tap *tap)
{
	struct mips32_common *mips32 = &mips_mAptiv->mips32;

	mips_mAptiv->common_magic = MIPSMAPTIV_COMMON_MAGIC;

	/* initialize mips4k specific info */
	mips32_init_arch_info(target, mips32, tap);
	mips32->arch_info = mips_mAptiv;

	return ERROR_OK;
}

static int mips_mAptiv_target_create(struct target *target, Jim_Interp *interp)
{
	struct mips_mAptiv_common *mips_mAptiv = calloc(1, sizeof(struct mips_mAptiv_common));

	mips_mAptiv_init_arch_info(target, mips_mAptiv, target->tap);

	return ERROR_OK;
}

static int mips_mAptiv_verify_pointer(struct command_context *cmd_ctx,
		struct mips_mAptiv_common *mips_mAptiv)
{
	if (mips_mAptiv->common_magic != MIPSMAPTIV_COMMON_MAGIC) {
		command_print(cmd_ctx, "target is not an MIPS_MAPTIV");
		return ERROR_TARGET_INVALID;
	}
	return ERROR_OK;
}

static const struct command_registration mips_mAptiv_exec_command_handlers[] = {
	COMMAND_REGISTRATION_DONE
};

const struct command_registration mips_mAptiv_command_handlers[] = {
	{
		.chain = mips32_command_handlers,
	},
	COMMAND_REGISTRATION_DONE
};

struct target_type mips_mAptiv_target = {
	.name = "mips_mAptiv",

	.poll = mips_common_poll,
	.arch_state = mips32_arch_state,

	.halt = mips_common_halt,
	.resume = mips_common_resume,
	.step = mips_common_step,

	.assert_reset = mips_common_assert_reset,
	.deassert_reset = mips_common_deassert_reset,

	.get_gdb_reg_list = mips32_get_gdb_reg_list,

	.read_memory = mips_common_read_memory,
	.write_memory = mips_common_write_memory,
	.checksum_memory = mips32_checksum_memory,
	.blank_check_memory = mips32_blank_check_memory,

	.run_algorithm = mips32_run_algorithm,

	.add_breakpoint = mips_mAptiv_add_breakpoint,
	.remove_breakpoint = mips_mAptiv_remove_breakpoint,
	.add_watchpoint = mips_common_add_watchpoint,
	.remove_watchpoint = mips_common_remove_watchpoint,

	.commands = mips_mAptiv_command_handlers,
	.target_create = mips_mAptiv_target_create,
	.init_target = mips_common_init_target,
	.examine = mips_common_examine,
};
