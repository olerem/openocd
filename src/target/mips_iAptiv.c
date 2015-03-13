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
 *                                                                         *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "breakpoints.h"
#include "mips32.h"
#include "mips_m4k.h"
#include "mips_iAptiv.h"
#include "mips_common.h"
#include "mips32_dmaacc.h"
#include "target_type.h"
#include "register.h"

static int mips_iAptiv_init_arch_info(struct target *target,
		struct mips_iAptiv_common *mips_iAptiv, struct jtag_tap *tap)
{
	struct mips32_common *mips32 = &mips_iAptiv->mips32;

	mips_iAptiv->common_magic = MIPS_IAPTIV_COMMON_MAGIC;

	/* initialize mips4k specific info */
	mips32_init_arch_info(target, mips32, tap);
	mips32->arch_info = mips_iAptiv;

	return ERROR_OK;
}

static int mips_iAptiv_target_create(struct target *target, Jim_Interp *interp)
{
	struct mips_iAptiv_common *mips_iAptiv = calloc(1, sizeof(struct mips_iAptiv_common));

	mips_iAptiv_init_arch_info(target, mips_iAptiv, target->tap);

	return ERROR_OK;
}

static int mips_IA_verify_pointer(struct command_context *cmd_ctx,
				 struct mips32_common *mips32)
{
	if (mips32->common_magic != MIPS32_COMMON_MAGIC) {
		command_print(cmd_ctx, "target is not an MIPS32");
		return ERROR_TARGET_INVALID;
	}
	return ERROR_OK;
}

int mips_IA_handle_cp0_command(struct command_invocation *cmd)
{
	int retval;
	struct target *target = get_current_target(CMD_CTX);
	struct mips32_common *mips32 = target_to_mips32(target);
	struct mips_ejtag *ejtag_info = &mips32->ejtag_info;

	retval = mips_IA_verify_pointer(CMD_CTX, mips32);
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
				retval = mips32_cp0_read(ejtag_info, &value, mips32_iA_cp0_regs[i].reg, mips32_iA_cp0_regs[i].sel);
				if (retval != ERROR_OK) {
					command_print(CMD_CTX, "couldn't access reg %s", mips32_iA_cp0_regs[i].name);
					return ERROR_OK;
				}

				command_print(CMD_CTX, "%*s: 0x%8.8x", 14, mips32_iA_cp0_regs[i].name, value);
			}
		} else {
			for (int i = 0; i < MIPS32NUMCP0REGS; i++) {
				/* find register name */
				if (strcmp(mips32_iA_cp0_regs[i].name, CMD_ARGV[0]) == 0) {
					retval = mips32_cp0_read(ejtag_info, &value, mips32_iA_cp0_regs[i].reg, mips32_iA_cp0_regs[i].sel);
					command_print(CMD_CTX, "0x%8.8x", value);
					return ERROR_OK;
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
				for (int i = 0; i < MIPS32NUMCP0REGS; i++) {
					/* find register name */
					if (strcmp(mips32_iA_cp0_regs[i].name, CMD_ARGV[0]) == 0) {
						COMMAND_PARSE_NUMBER(u32, CMD_ARGV[1], value);
						retval = mips32_cp0_write(ejtag_info, value, mips32_iA_cp0_regs[i].reg, mips32_iA_cp0_regs[i].sel);
						return ERROR_OK;
					}
				}

				LOG_ERROR("BUG: register '%s' not found", CMD_ARGV[0]);
				return ERROR_COMMAND_SYNTAX_ERROR;
			} else {
				uint32_t cp0_reg, cp0_sel;
				COMMAND_PARSE_NUMBER(u32, CMD_ARGV[0], cp0_reg);
				COMMAND_PARSE_NUMBER(u32, CMD_ARGV[1], cp0_sel);

				retval = mips32_cp0_read(ejtag_info, &value, cp0_reg, cp0_sel);
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
			retval = mips32_cp0_write(ejtag_info, value, cp0_reg, cp0_sel);
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


static const struct command_registration mips_iAptiv_exec_command_handlers[] = {
	{
		.name = "cp0",
		.handler = mips_IA_handle_cp0_command,
		.mode = COMMAND_EXEC,
		.help = "display/modify cp0 register(s)",
		.usage = "[[reg_name|regnum select] [value]]",
	},
	COMMAND_REGISTRATION_DONE
};

const struct command_registration mips_iAptiv_command_handlers[] = {
	{
		.chain = mips32_command_handlers,
	},
	COMMAND_REGISTRATION_DONE
};

struct target_type mips_iAptiv_target = {
	.name = "mips_iAptiv",

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

	.add_breakpoint = mips_common_add_breakpoint,
	.remove_breakpoint = mips_common_remove_breakpoint,
	.add_watchpoint = mips_common_add_watchpoint,
	.remove_watchpoint = mips_common_remove_watchpoint,

	.commands = mips_iAptiv_command_handlers,
	.target_create = mips_iAptiv_target_create,
	.init_target = mips_common_init_target,
	.examine = mips_common_examine,
};
