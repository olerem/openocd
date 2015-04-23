/***************************************************************************
 *   Copyright (C) 2015 by Oleksij Rempel                                  *
 *   linux@rempel-privat.de                                                *
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
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "jtag/interface.h"
#include "arm.h"
#include "armv7a.h"
#include "arm7a_cache.h"
#include <helper/time_support.h>
#include "target.h"
#include "target_type.h"

#define L2X0_CLEAN_INV_WAY              0x7FC

/* L2 is not specific to armv7a  a specific file is needed */
static int arm7a_l2x_inval_all(struct target *target)
{
	struct armv7a_common *armv7a = target_to_armv7a(target);
	struct armv7a_l2x_cache *l2x_cache = (struct armv7a_l2x_cache *)
		(armv7a->armv7a_mmu.armv7a_cache.l2_cache);
	uint32_t base = l2x_cache->base;
	uint32_t l2_way = l2x_cache->way;
	uint32_t l2_way_val = (1 << l2_way) - 1;
	int retval;

	retval = target_write_phys_memory(target,
			base + L2X0_CLEAN_INV_WAY,
			4, 1, (uint8_t *)&l2_way_val);
	return retval;
}

static int arm7a_handle_l2x_cache_info_command(struct command_context *cmd_ctx,
	struct armv7a_cache_common *armv7a_cache)
{
	struct armv7a_l2x_cache *l2x_cache = (struct armv7a_l2x_cache *)
		(armv7a_cache->l2_cache);

	if (armv7a_cache->ctype == -1) {
		command_print(cmd_ctx, "cache not yet identified");
		return ERROR_OK;
	}

	command_print(cmd_ctx,
		      "L2 unified cache Base Address 0x%" PRIx32 ", %" PRId32 " ways",
		      l2x_cache->base, l2x_cache->way);

	return ERROR_OK;
}

COMMAND_HANDLER(arm7a_l2x_cache_info_command)
{
	struct target *target = get_current_target(CMD_CTX);
	struct armv7a_common *armv7a = target_to_armv7a(target);

	return arm7a_handle_l2x_cache_info_command(CMD_CTX,
			&armv7a->armv7a_mmu.armv7a_cache);
}

COMMAND_HANDLER(arm7a_l2x_cache_inval_all_command)
{
	struct target *target = get_current_target(CMD_CTX);

	return arm7a_l2x_inval_all(target);
}

static const struct command_registration arm7a_l2x_cache_commands[] = {
	{
		.name = "info",
		.handler = arm7a_l2x_cache_info_command,
		.mode = COMMAND_ANY,
		.help = "print cache realted information",
		.usage = "",
	},
	{
		.name = "inval_all",
		.handler = arm7a_l2x_cache_inval_all_command,
		.mode = COMMAND_ANY,
		.help = "invalidate complete l2x cache",
		.usage = "",
	},
	COMMAND_REGISTRATION_DONE
};

const struct command_registration arm7a_l2x_cache_command_handler[] = {
	{
		.name = "l2x",
		.mode = COMMAND_ANY,
		.help = "l2x cache command group",
		.usage = "",
		.chain = arm7a_l2x_cache_commands,
	},
	COMMAND_REGISTRATION_DONE
};
