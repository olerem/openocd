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
#include "mips_mAptiv.h"
#include "mips_common.h"
#include "mips32_dmaacc.h"
#include "target_type.h"
#include "register.h"

static int mips_mAptiv_init_arch_info(struct target *target,
		struct mips_mAptiv_common *mips_mAptiv, struct jtag_tap *tap)
{
	struct mips32_common *mips32 = &mips_mAptiv->mips32;

	mips_mAptiv->common_magic = MIPSMAPTIV_COMMON_MAGIC;

	/* initialize mips specific info */
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

	.add_breakpoint = mips_common_add_breakpoint,
	.remove_breakpoint = mips_common_remove_breakpoint,
	.add_watchpoint = mips_common_add_watchpoint,
	.remove_watchpoint = mips_common_remove_watchpoint,

	.commands = mips_mAptiv_command_handlers,
	.target_create = mips_mAptiv_target_create,
	.init_target = mips_common_init_target,
	.examine = mips_common_examine,
};
