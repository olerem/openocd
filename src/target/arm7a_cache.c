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

COMMAND_HANDLER(arm7a_cache_info_command)
{
	struct target *target = get_current_target(CMD_CTX);
	struct armv7a_common *armv7a = target_to_armv7a(target);

	return armv7a_handle_cache_info_command(CMD_CTX,
			&armv7a->armv7a_mmu.armv7a_cache);
}

static const struct command_registration arm7a_cache_commands[] = {
	{
		.name = "info",
		.handler = arm7a_cache_info_command,
		.mode = COMMAND_ANY,
		.help = "print cache realted information",
		.usage = "",
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
