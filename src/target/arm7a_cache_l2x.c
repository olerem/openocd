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

/*
 * clean and invalidate complete l2x cache
 */
static int arm7a_l2x_flush_all_data(struct target *target)
{
	struct armv7a_common *armv7a = target_to_armv7a(target);
	struct armv7a_l2x_cache *l2x_cache = (struct armv7a_l2x_cache *)
		(armv7a->armv7a_mmu.armv7a_cache.l2_cache);
	uint32_t l2_way_val = (1 << l2x_cache->way) - 1;

	return target_write_phys_memory(target,
			l2x_cache->base + L2X0_CLEAN_INV_WAY,
			4, 1, (uint8_t *)&l2_way_val);
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

static const struct l2c_init_data of_l2c310_data __initconst = {
	.type = "L2C-310",
	.way_size_0 = SZ_8K,
	.num_lock = 8,

	.enable = l2c310_enable,
	.fixup = l2c310_fixup,
	.save  = l2c310_save,
	.configure = l2c310_configure,
	.outer_cache = {
		.inv_range   = l2c210_inv_range,
		.clean_range = l2c210_clean_range,
		.flush_range = l2c210_flush_range,
		.flush_all   = l2c210_flush_all,
		.disable     = l2c310_disable,
		.sync        = l2c210_sync,
		.resume      = l2c310_resume,
	},
};

static const struct l2c_init_data of_l2c310_coherent_data __initconst = {
	.type = "L2C-310 Coherent",
	.way_size_0 = SZ_8K,
	.num_lock = 8,

	.enable = l2c310_enable,
	.fixup = l2c310_fixup,
	.save  = l2c310_save,
	.configure = l2c310_configure,
	.outer_cache = {
		.inv_range   = l2c210_inv_range,
		.clean_range = l2c210_clean_range,
		.flush_range = l2c210_flush_range,
		.flush_all   = l2c210_flush_all,
		.disable     = l2c310_disable,
		.resume      = l2c310_resume,
	},
};

static const struct l2c_init_data of_aurora_with_outer_data __initconst = {
	.type = "Aurora",
	.way_size_0 = SZ_4K,
	.num_lock = 4,

	.enable = l2c_enable,
	.fixup = aurora_fixup,
	.save  = aurora_save,
	.outer_cache = {
		.inv_range   = aurora_inv_range,
		.clean_range = aurora_clean_range,
		.flush_range = aurora_flush_range,
		.flush_all   = aurora_flush_all,
		.disable     = aurora_disable,
		.sync	     = aurora_cache_sync,
		.resume      = l2c_resume,
	},
};

static const struct l2c_init_data of_aurora_no_outer_data __initconst = {
	.type = "Aurora",
	.way_size_0 = SZ_4K,
	.num_lock = 4,

	.enable = aurora_enable_no_outer,
	.fixup = aurora_fixup,
	.save  = aurora_save,
	.outer_cache = {
		.resume      = l2c_resume,
	},
};

static const struct l2c_init_data of_bcm_l2x0_data __initconst = {
	.type = "BCM-L2C-310",
	.way_size_0 = SZ_8K,
	.num_lock = 8,

	.enable = l2c310_enable,
	.save  = l2c310_save,
	.configure = l2c310_configure,
	.outer_cache = {
		.inv_range   = bcm_inv_range,
		.clean_range = bcm_clean_range,
		.flush_range = bcm_flush_range,
		.flush_all   = l2c210_flush_all,
		.disable     = l2c310_disable,
		.sync        = l2c210_sync,
		.resume      = l2c310_resume,
	},
};

static const struct l2c_init_data of_l2c210_data = {
	.type = "L2C-210",
	.way_size_0 = SZ_8K,
	.num_lock = 1,

	.enable = l2c_enable,
	.save = l2c_save,
	.outer_cache = {
		.inv_range   = l2c210_inv_range,
		.clean_range = l2c210_clean_range,
		.flush_range = l2c210_flush_range,
		.flush_all   = l2c210_flush_all,
		.disable     = l2c_disable,
		.sync        = l2c210_sync,
		.resume      = l2c_resume,
	},
};

static const struct l2c_init_data of_l2c220_data = {
	.type = "L2C-220",
	.way_size_0 = SZ_8K,
	.num_lock = 1,

	.enable = l2c220_enable,
	.save = l2c_save,
	.outer_cache = {
		.inv_range   = l2c220_inv_range,
		.clean_range = l2c220_clean_range,
		.flush_range = l2c220_flush_range,
		.flush_all   = l2c220_flush_all,
		.disable     = l2c_disable,
		.sync        = l2c220_sync,
		.resume      = l2c_resume,
	},
};

COMMAND_HANDLER(arm7a_l2x_cache_info_command)
{
	struct target *target = get_current_target(CMD_CTX);
	struct armv7a_common *armv7a = target_to_armv7a(target);

	return arm7a_handle_l2x_cache_info_command(CMD_CTX,
			&armv7a->armv7a_mmu.armv7a_cache);
}

COMMAND_HANDLER(arm7a_l2x_cache_flash_all_command)
{
	struct target *target = get_current_target(CMD_CTX);

	return arm7a_l2x_flush_all_data(target);
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
		.name = "flash_all",
		.handler = arm7a_l2x_cache_flash_all_command,
		.mode = COMMAND_ANY,
		.help = "flash complete l2x cache",
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
