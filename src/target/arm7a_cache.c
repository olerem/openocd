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
#include "arm_opcodes.h"

static int armv7a_d_cache_clean_inval_all(struct target *target)
{
	struct armv7a_common *armv7a = target_to_armv7a(target);
	struct arm_dpm *dpm = armv7a->arm.dpm;
	struct armv7a_cachesize *d_u_size =
		&(armv7a->armv7a_mmu.armv7a_cache.d_u_size);
	int32_t c_way, c_index = d_u_size->index;
	int retval;

	/*  check that cache data is on at target halt */
	if (!armv7a->armv7a_mmu.armv7a_cache.d_u_cache_enabled) {
		LOG_INFO("flushed not performed :cache not on at target halt");
		return ERROR_OK;
	}

	retval = dpm->prepare(dpm);
	if (retval != ERROR_OK)
		goto done;

	do {
		c_way = d_u_size->way;
		do {
			uint32_t value = (c_index << d_u_size->index_shift)
				| (c_way << d_u_size->way_shift);
			/*  DCCISW */
			/* LOG_INFO ("%d %d %x",c_way,c_index,value); */
			retval = dpm->instr_write_data_r0(dpm,
					ARMV4_5_MCR(15, 0, 0, 7, 14, 2),
					value);
			if (retval != ERROR_OK)
				goto done;
			c_way -= 1;
		} while (c_way >= 0);
		c_index -= 1;
	} while (c_index >= 0);

	return retval;

done:
	LOG_ERROR("clean invalidate failed");
	dpm->finish(dpm);

	return retval;
}

static int armv7a_d_cache_inval_virt(struct target *target, uint32_t virt,
					unsigned int size)
{
	struct armv7a_common *armv7a = target_to_armv7a(target);
	struct arm_dpm *dpm = armv7a->arm.dpm;
	int retval;

	/*  check that cache data is on at target halt */
	if (!armv7a->armv7a_mmu.armv7a_cache.d_u_cache_enabled) {
		LOG_INFO("flushed not performed :cache not on at target halt");
		return ERROR_OK;
	}

	retval = dpm->prepare(dpm);
	if (retval != ERROR_OK)
		goto done;

	retval = dpm->instr_write_data_r0(dpm,
				ARMV4_5_MCR(15, 0, 0, 7, 6, 1), virt);
	if (retval != ERROR_OK)
		goto done;
	return retval;

done:
	LOG_ERROR("d-cache invalidate failed");
	dpm->finish(dpm);

	return retval;
}

static int armv7a_d_cache_clean_virt(struct target *target, uint32_t virt,
					unsigned int size)
{
	struct armv7a_common *armv7a = target_to_armv7a(target);
	struct arm_dpm *dpm = armv7a->arm.dpm;
	int retval;

	/*  check that cache data is on at target halt */
	if (!armv7a->armv7a_mmu.armv7a_cache.d_u_cache_enabled) {
		LOG_INFO("flushed not performed :cache not on at target halt");
		return ERROR_OK;
	}

	retval = dpm->prepare(dpm);
	if (retval != ERROR_OK)
		goto done;

	/* FIXME: do we need DCCVAC or DCCVAU */
	retval = dpm->instr_write_data_r0(dpm,
				ARMV4_5_MCR(15, 0, 0, 7, 10, 1), virt);
	if (retval != ERROR_OK)
		goto done;
	return retval;

done:
	LOG_ERROR("d-cache invalidate failed");
	dpm->finish(dpm);

	return retval;
}

COMMAND_HANDLER(arm7a_cache_info_command)
{
	struct target *target = get_current_target(CMD_CTX);
	struct armv7a_common *armv7a = target_to_armv7a(target);

	return armv7a_handle_cache_info_command(CMD_CTX,
			&armv7a->armv7a_mmu.armv7a_cache);
}

COMMAND_HANDLER(arm7a_cache_flash_all_command)
{
	struct target *target = get_current_target(CMD_CTX);

	armv7a_d_cache_clean_inval_all(target);

	return 0;
}

COMMAND_HANDLER(arm7a_cache_inval_virt_command)
{
	struct target *target = get_current_target(CMD_CTX);
	uint32_t virt, size;

	if (CMD_ARGC != 1)
		return ERROR_COMMAND_SYNTAX_ERROR;
	else if (CMD_ARGC == 2)
		COMMAND_PARSE_NUMBER(u32, CMD_ARGV[1], size);

	COMMAND_PARSE_NUMBER(u32, CMD_ARGV[0], virt);

	return armv7a_d_cache_inval_virt(target, virt, size);
}

COMMAND_HANDLER(arm7a_cache_clean_virt_command)
{
	struct target *target = get_current_target(CMD_CTX);
	uint32_t virt, size;

	if (CMD_ARGC != 1)
		return ERROR_COMMAND_SYNTAX_ERROR;
	else if (CMD_ARGC == 2)
		COMMAND_PARSE_NUMBER(u32, CMD_ARGV[1], size);

	COMMAND_PARSE_NUMBER(u32, CMD_ARGV[0], virt);

	return armv7a_d_cache_clean_virt(target, virt, size);
}

static const struct command_registration arm7a_cache_commands[] = {
	{
		.name = "info",
		.handler = arm7a_cache_info_command,
		.mode = COMMAND_ANY,
		.help = "print cache realted information",
		.usage = "",
	},
	{
		.name = "flash_all",
		.handler = arm7a_cache_flash_all_command,
		.mode = COMMAND_ANY,
		.help = "flash (clean and invalidate) all l1 caches",
		.usage = "",
	},
	{
		.name = "inval",
		.handler = arm7a_cache_inval_virt_command,
		.mode = COMMAND_ANY,
		.help = "invalidate l1 by virtual address offset and range size",
		.usage = "<virt_addr> [size]",
	},
	{
		.name = "clean",
		.handler = arm7a_cache_clean_virt_command,
		.mode = COMMAND_ANY,
		.help = "clean l1 by virtual address address offset and range size",
		.usage = "<virt_addr> [size]",
	},
	{
		.chain = arm7a_l2x_cache_command_handler,
	},
	COMMAND_REGISTRATION_DONE
};

const struct command_registration arm7a_cache_command_handlers[] = {
	{
		.name = "cache",
		.mode = COMMAND_ANY,
		.help = "cache command group",
		.usage = "",
		.chain = arm7a_cache_commands,
	},
	COMMAND_REGISTRATION_DONE
};
