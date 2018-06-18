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
 *	 Copyright (C) 2014 by Kent Brinkley								   *
 *	 jkbrinkley_imgtec@gmail.com										   *
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

static int mips_m4k_init_arch_info(struct target *target,
		struct mips_m4k_common *mips_m4k, struct jtag_tap *tap)
{
	struct mips32_common *mips32 = &mips_m4k->mips32;
	mips_m4k->common_magic = MIPSM4K_COMMON_MAGIC;

	/* initialize mips4k specific info */
	mips32_init_arch_info(target, mips32, tap);
	mips32->arch_info = mips_m4k;
	mips32->cp0_mask = MIPS_CP0_MK4;

	return ERROR_OK;
}

static int mips_m4k_target_create(struct target *target, Jim_Interp *interp)
{
	struct mips_m4k_common *mips_m4k = calloc(1, sizeof(struct mips_m4k_common));

	mips_m4k_init_arch_info(target, mips_m4k, target->tap);

	return ERROR_OK;
}

COMMAND_HANDLER(mips_m4k_handle_cp0_command)
{
	/* Call common code - maintaining backward compatibility */
	return mips32_cp0_command(cmd);

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
	return mips32_scan_delay_command(cmd);

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

	.add_breakpoint = mips_common_add_breakpoint,
	.remove_breakpoint = mips_common_remove_breakpoint,
	.add_watchpoint = mips_common_add_watchpoint,
	.remove_watchpoint = mips_common_remove_watchpoint,

	.commands = mips_m4k_command_handlers,
	.target_create = mips_m4k_target_create,
	.init_target = mips_common_init_target,
	.examine = mips_common_examine,
};
