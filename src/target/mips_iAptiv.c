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
#include "mips_common.h"
#include "mips_iAptiv.h"
#include "mips32_dmaacc.h"
#include "target_type.h"
#include "register.h"


#define CMDALL_HALT 0
static const struct {
	unsigned option;
	const char *arg;
} cmdall_cmd_list[1] = {
	{ CMDALL_HALT, "halt"},
};

static int mips_iAptiv_init_arch_info(struct target *target,
		struct mips_iAptiv_common *mips_iAptiv, struct jtag_tap *tap)
{
	struct mips32_common *mips32 = &mips_iAptiv->mips32;

	mips_iAptiv->common_magic = MIPS_IAPTIV_COMMON_MAGIC;

	/* initialize mips4k specific info */
	mips32_init_arch_info(target, mips32, tap);
	mips32->arch_info = mips_iAptiv;
	mips32->cp0_mask = MIPS_CP0_iAPTIV;

	return ERROR_OK;
}

static int mips_iAptiv_target_create(struct target *target, Jim_Interp *interp)
{
	struct mips_iAptiv_common *mips_iAptiv = calloc(1, sizeof(struct mips_iAptiv_common));

	mips_iAptiv_init_arch_info(target, mips_iAptiv, target->tap);

	return ERROR_OK;
}

static int mips_iAptiv_halt_all(struct target *target)
{
	struct mips32_common *mips32 = target_to_mips32(target);
	struct mips_ejtag *ejtag_info = &mips32->ejtag_info;
	uint32_t ejtag_ctrl;

	int retval = ERROR_OK;
	struct target *curr;

	do {
		int ret = ERROR_OK;
		curr = target;
		if (curr->state != TARGET_HALTED) {
			LOG_INFO("halt core");
			mips32 = target_to_mips32(curr);
			ejtag_info = &mips32->ejtag_info;
			ret = mips_ejtag_enter_debug(ejtag_info);
			if (ret != ERROR_OK) {
				LOG_ERROR("halt failed target: %s", curr->cmd_name);
				retval = ret;
			}
		}
		target = target->next;
	} while (curr->next != (struct target *)NULL);
	return retval;
}

COMMAND_HANDLER(mips_ia_handle_cmdall_command)
{
	struct target *target = get_current_target(CMD_CTX);
	struct mips32_common *mips32 = target_to_mips32(target);
//	struct mips_ejtag *ejtag_info = &mips_iAptiv->mips32.ejtag_info;

	int retval = -1;
	int i = 0;

	if ((CMD_ARGC >= 2) || (CMD_ARGC == 0)){
		LOG_DEBUG("ERROR_COMMAND_SYNTAX_ERROR");
		return ERROR_COMMAND_SYNTAX_ERROR;
	}

	if (CMD_ARGC == 1) {
		for (i = 0; i < 1 ; i++) {
			if (strcmp(CMD_ARGV[0], cmdall_cmd_list[i].arg) == 0) {
				switch (i) {
					case CMDALL_HALT:
						LOG_INFO ("cmdall halt");
						retval = mips_iAptiv_halt_all(target);
						return retval;
						break;
					default:
						LOG_ERROR("Invalid cmdall command '%s' not found", CMD_ARGV[0]);
						return ERROR_COMMAND_SYNTAX_ERROR;
				}
			} else {
					LOG_ERROR("Invalid cmdall command '%s' not found", CMD_ARGV[0]);
					return ERROR_COMMAND_SYNTAX_ERROR;
			}
		}
	}
	
	return ERROR_OK;
}

static const struct command_registration mips_iAptiv_exec_command_handlers[] = {
	{
		.name = "cmdall",
		.handler = mips_ia_handle_cmdall_command,
		.mode = COMMAND_EXEC,
		.usage = "cmdall halt",
		.help = "cmdall allows any command to be executed by all devices",
	},
	COMMAND_REGISTRATION_DONE
};

const struct command_registration mips_iAptiv_command_handlers[] = {
	{
		.chain = mips32_command_handlers,
	},
	{
		.name = "mips_iA",
		.mode = COMMAND_ANY,
		.help = "mips_iA command group",
		.usage = "",
		.chain = mips_iAptiv_exec_command_handlers,
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
