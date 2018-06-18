/***************************************************************************
 *	 Copyright (C) 2008 by Spencer Oliver								   *
 *	 spen@spen-soft.co.uk												   *
 *																		   *
 *	 Copyright (C) 2008 by David T.L. Wong								   *
 *																		   *
 *	 Copyright (C) 2009 by David N. Claffey <dnclaffey@gmail.com>		   *
 *																		   *
 *	 Copyright (C) 2011 by Drasko DRASKOVIC								   *
 *	 drasko.draskovic@gmail.com											   *
 *																		   *
 *	 This program is free software; you can redistribute it and/or modify  *
 *	 it under the terms of the GNU General Public License as published by  *
 *	 the Free Software Foundation; either version 2 of the License, or	   *
 *	 (at your option) any later version.								   *
 *																		   *
 *	 This program is distributed in the hope that it will be useful,	   *
 *	 but WITHOUT ANY WARRANTY; without even the implied warranty of		   *
 *	 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the		   *
 *	 GNU General Public License for more details.						   *
 *																		   *
 *	 You should have received a copy of the GNU General Public License	   *
 *	 along with this program; if not, write to the						   *
 *	 Free Software Foundation, Inc.,									   *
 *	 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.		   *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "breakpoints.h"
#include "mips32.h"
#include "mips_m4k.h"
#include "mips_common.h"
#include "mips32_dmaacc.h"
#include "target_type.h"
#include "register.h"

static int mips_m4k_set_breakpoint(struct target *target,
		struct breakpoint *breakpoint)
{
	struct mips32_common *mips32 = target_to_mips32(target);
	struct mips_ejtag *ejtag_info = &mips32->ejtag_info;
	struct mips32_comparator *comparator_list = mips32->inst_break_list;
	int retval;

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
		comparator_list[bp_num].bp_value = breakpoint->address;

		/* EJTAG 2.0 uses 30bit IBA. First 2 bits are reserved.
		 * Warning: there is no IB ASID registers in 2.0.
		 * Do not set it! :) */
		if (ejtag_info->ejtag_version == EJTAG_VERSION_20)
			comparator_list[bp_num].bp_value &= 0xFFFFFFFC;

		/* Check for microMips and executing in MIPS16 ISA */
		target_write_u32(target, comparator_list[bp_num].reg_address,
						 comparator_list[bp_num].bp_value);
		
		target_write_u32(target, comparator_list[bp_num].reg_address +
				 ejtag_info->ejtag_ibm_offs, 0x00000000);

		target_write_u32(target, comparator_list[bp_num].reg_address +
				 ejtag_info->ejtag_ibc_offs, 1);
		LOG_DEBUG("bpid: %" PRIu32 ", bp_num %i bp_value 0x%" PRIx32 "",
				  breakpoint->unique_id,
				  bp_num, comparator_list[bp_num].bp_value);

	} else if (breakpoint->type == BKPT_SOFT) {
		LOG_DEBUG("bpid: %" PRIu32, breakpoint->unique_id);

		if (breakpoint->length == 4) {
			uint32_t verify = 0xffffffff;

			retval = target_read_memory(target, breakpoint->address, breakpoint->length, 1,
										breakpoint->orig_instr);
			if (retval != ERROR_OK) {
				LOG_DEBUG("target_read_memory failed");
				return retval;
			}

			retval = target_write_u32(target, breakpoint->address, MIPS32_SDBBP);
			if (retval != ERROR_OK) {
				LOG_DEBUG("target_write_u32 failed");
				return retval;
			}

			retval = target_read_u32(target, breakpoint->address, &verify);
			if (retval != ERROR_OK) {
				LOG_DEBUG("target_read_u32 failed");
				return retval;
			}

			if (verify != MIPS32_SDBBP) {
				LOG_ERROR("Unable to set 32bit breakpoint at address %08" PRIx32
						  " - check that memory is read/writable", breakpoint->address);
				return ERROR_OK;
			}
		} else {
			uint16_t verify = 0xffff;

			retval = target_read_memory(target, breakpoint->address, breakpoint->length, 1,
										breakpoint->orig_instr);
			if (retval != ERROR_OK) {
				LOG_DEBUG("target_read_memory failed");
				return retval;
			}

			retval = target_write_u16(target, breakpoint->address, MIPS16_SDBBP);
			if (retval != ERROR_OK) {
				LOG_DEBUG("target_write_u16 failed");
				return retval;
			}

			retval = target_read_u16(target, breakpoint->address, &verify);
			if (retval != ERROR_OK) {
				LOG_DEBUG("target_read_u16 failed");
				return retval;
			}

			if (verify != MIPS16_SDBBP) {
				LOG_ERROR("Unable to set 16bit breakpoint at address %08" PRIx32
						  " - check that memory is read/writable", breakpoint->address);
				return ERROR_OK;
			}
		}

		breakpoint->set = 20; /* Any nice value but 0 */
	}

	return ERROR_OK;
}

static int mips_m4k_unset_breakpoint(struct target *target,
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

		LOG_DEBUG("bpid: %" PRIu32 " - releasing hw: %d",
				breakpoint->unique_id,
				bp_num);

		comparator_list[bp_num].used = 0;
		comparator_list[bp_num].bp_value = 0;

		target_write_u32(target, comparator_list[bp_num].reg_address +
				 ejtag_info->ejtag_ibc_offs, 0);
	} else {
		/* restore original instruction (kept in target endianness) */
		LOG_DEBUG("bpid: %" PRIu32, breakpoint->unique_id);
		if (breakpoint->length == 4) {
			uint32_t current_instr;

			/* check that user program has not modified breakpoint instruction */
			/* Remove isa_mode info from length of read */
			retval = target_read_memory(target, breakpoint->address, 4,	1,
										(uint8_t *)&current_instr);
			if (retval != ERROR_OK) {
				LOG_DEBUG("target_read_memory failed");
				return retval;
			}

			/**
			 * target_read_memory() gets us data in _target_ endianess.
			 * If we want to use this data on the host for comparisons with some macros
			 * we must first transform it to _host_ endianess using target_buffer_get_u32().
			 */
			current_instr = target_buffer_get_u32(target, (uint8_t *)&current_instr);

			if (current_instr == MIPS32_SDBBP) {
				retval = target_write_memory(target, breakpoint->address, 4, 1,
											 breakpoint->orig_instr);
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
			retval = target_read_memory(target, breakpoint->address, 2, 1,
										(uint8_t *)&current_instr);
			if (retval != ERROR_OK) {
				LOG_DEBUG("target_read_memory failed");
				return retval;
			}

			current_instr = target_buffer_get_u16(target, (uint8_t *)&current_instr);
			if (current_instr == MIPS16_SDBBP) {
				retval = target_write_memory(target, breakpoint->address, 2, 1,
											 breakpoint->orig_instr);
				if (retval != ERROR_OK) {
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

static int mips_m4k_add_breakpoint(struct target *target, struct breakpoint *breakpoint)
{
	struct mips32_common *mips32 = target_to_mips32(target);

	if (breakpoint->type == BKPT_HARD) {
		if (mips32->num_inst_bpoints_avail < 1) {
			LOG_INFO("no hardware breakpoint available");
			return ERROR_TARGET_RESOURCE_NOT_AVAILABLE;
		}

		mips32->num_inst_bpoints_avail--;
	}

	return mips_m4k_set_breakpoint(target, breakpoint);
}

static int mips_m4k_remove_breakpoint(struct target *target,
									  struct breakpoint *breakpoint)
{
	/* get pointers to arch-specific information */
	struct mips32_common *mips32 = target_to_mips32(target);

	if (target->state != TARGET_HALTED) {
		LOG_WARNING("target not halted");
		return ERROR_TARGET_NOT_HALTED;
	}

	if (breakpoint->set)
		mips_m4k_unset_breakpoint(target, breakpoint);

	if (breakpoint->type == BKPT_HARD)
		mips32->num_inst_bpoints_avail++;

	return ERROR_OK;
}

static int mips_m4k_init_arch_info(struct target *target,
		struct mips_m4k_common *mips_m4k, struct jtag_tap *tap)
{
	struct mips32_common *mips32 = &mips_m4k->mips32;

	mips_m4k->common_magic = MIPSM4K_COMMON_MAGIC;

	/* initialize mips4k specific info */
	mips32_init_arch_info(target, mips32, tap);
	mips32->arch_info = mips_m4k;

	return ERROR_OK;
}


static int mips_m4k_target_create(struct target *target, Jim_Interp *interp)
{
	struct mips_m4k_common *mips_m4k = calloc(1, sizeof(struct mips_m4k_common));

	mips_m4k_init_arch_info(target, mips_m4k, target->tap);

	return ERROR_OK;
}

static int mips_m4k_verify_pointer(struct command_context *cmd_ctx,
		struct mips_m4k_common *mips_m4k)
{
	if (mips_m4k->common_magic != MIPSM4K_COMMON_MAGIC) {
		command_print(cmd_ctx, "target is not an MIPS_M4K");
		return ERROR_TARGET_INVALID;
	}
	return ERROR_OK;
}

COMMAND_HANDLER(mips_m4k_handle_cp0_command)
{
	/* Call common code - maintaining backward compatibility */
	return (mips32_cp0_command (cmd));

}

COMMAND_HANDLER(mips_m4k_handle_smp_off_command)
{
	struct target *target = get_current_target(CMD_CTX);
	/* check target is an smp target */
	struct target_list *head;
	struct target *curr;
	head = target->head;
	target->smp = 0;
	if (head != (struct target_list *)NULL) {
		while (head != (struct target_list *)NULL) {
			curr = head->target;
			curr->smp = 0;
			head = head->next;
		}
		/*	fixes the target display to the debugger */
		target->gdb_service->target = target;
	}
	return ERROR_OK;
}

COMMAND_HANDLER(mips_m4k_handle_smp_on_command)
{
	struct target *target = get_current_target(CMD_CTX);
	struct target_list *head;
	struct target *curr;
	head = target->head;
	if (head != (struct target_list *)NULL) {
		target->smp = 1;
		while (head != (struct target_list *)NULL) {
			curr = head->target;
			curr->smp = 1;
			head = head->next;
		}
	}
	return ERROR_OK;
}

COMMAND_HANDLER(mips_m4k_handle_smp_gdb_command)
{
	struct target *target = get_current_target(CMD_CTX);
	int retval = ERROR_OK;
	struct target_list *head;
	head = target->head;
	if (head != (struct target_list *)NULL) {
		if (CMD_ARGC == 1) {
			int coreid = 0;
			COMMAND_PARSE_NUMBER(int, CMD_ARGV[0], coreid);
			if (ERROR_OK != retval)
				return retval;
			target->gdb_service->core[1] = coreid;

		}
		command_print(CMD_CTX, "gdb coreid	%" PRId32 " -> %" PRId32, target->gdb_service->core[0]
			, target->gdb_service->core[1]);
	}
	return ERROR_OK;
}

COMMAND_HANDLER(mips_m4k_handle_scan_delay_command)
{

	/* Call common code - maintaining backward compatibility */
	return (mips32_scan_delay_command (cmd));

}

static const struct command_registration mips_m4k_exec_command_handlers[] = {
	{
		.name = "cp0",
		.handler = mips_m4k_handle_cp0_command,
		.mode = COMMAND_EXEC,
		.usage = "[[reg_name|regnum select] [value]]]",
		.help = "display/modify cp0 register",
	},
	{
		.name = "smp_off",
		.handler = mips_m4k_handle_smp_off_command,
		.mode = COMMAND_EXEC,
		.help = "Stop smp handling",
		.usage = "",},

	{
		.name = "smp_on",
		.handler = mips_m4k_handle_smp_on_command,
		.mode = COMMAND_EXEC,
		.help = "Restart smp handling",
		.usage = "",
	},
	{
		.name = "smp_gdb",
		.handler = mips_m4k_handle_smp_gdb_command,
		.mode = COMMAND_EXEC,
		.help = "display/fix current core played to gdb",
		.usage = "",
	},
	{
		.name = "scan_delay",
		.handler = mips_m4k_handle_scan_delay_command,
		.mode = COMMAND_ANY,
		.help = "display/set scan delay in nano seconds",
		.usage = "[value]",
	},

	COMMAND_REGISTRATION_DONE
};

const struct command_registration mips_m4k_command_handlers[] = {
	{
		.chain = mips32_command_handlers,
	},
	{
		.name = "mips_m4k",
		.mode = COMMAND_ANY,
		.help = "mips_m4k command group",
		.usage = "",
		.chain = mips_m4k_exec_command_handlers,
	},
	COMMAND_REGISTRATION_DONE
};

struct target_type mips_m4k_target = {
	.name = "mips_m4k",

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

	.add_breakpoint = mips_m4k_add_breakpoint,
	.remove_breakpoint = mips_m4k_remove_breakpoint,
	.add_watchpoint = mips_common_add_watchpoint,
	.remove_watchpoint = mips_common_remove_watchpoint,

	.commands = mips_m4k_command_handlers,
	.target_create = mips_m4k_target_create,
	.init_target = mips_common_init_target,
	.examine = mips_common_examine,
};
