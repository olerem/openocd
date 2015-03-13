/***************************************************************************
 *   Copyright (C) 2008 by Spencer Oliver                                  *
 *   spen@spen-soft.co.uk                                                  *
 *                                                                         *
 *   Copyright (C) 2008 by David T.L. Wong                                 *
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

#ifndef MIPS_COMMON_H
#define MIPS_COMMON_H

struct target;

#define MIPS_COMMON_MAGIC	0xB321B321

struct mips_common {
	uint32_t common_magic;
	bool is_pic32;
	struct mips32_common mips32;
};

inline struct mips_common *target_to_mips_common(struct target *target)
{
	return container_of(target->arch_info,
			struct mips_common, mips32);
}

int mips_common_add_breakpoint(struct target *target, struct breakpoint *breakpoint);
int mips_common_add_watchpoint(struct target *target, struct watchpoint *watchpoint);
int mips_common_assert_reset(struct target *target);
int mips_common_deassert_reset(struct target *target);
int mips_common_examine(struct target *target);
int mips_common_examine_debug_reason(struct target *target);
int mips_common_halt(struct target *target);
int mips_common_init_target(struct command_context *cmd_ctx,
							struct target *target);
int mips_common_poll(struct target *target);
int mips_common_resume(struct target *target, int current,
					   uint32_t address, int handle_breakpoints, int debug_execution);
int mips_common_read_memory(struct target *target, uint32_t address,
							uint32_t size, uint32_t count, uint8_t *buffer);
int mips_common_step(struct target *target, int current,
					 uint32_t address, int handle_breakpoints);
int mips_common_write_memory(struct target *target, uint32_t address,
							 uint32_t size, uint32_t count, const uint8_t *buffer);
int mips_common_remove_breakpoint(struct target *target, struct breakpoint *breakpoint);
int mips_common_remove_watchpoint(struct target *target, struct watchpoint *watchpoint);

/* Common commands define in both m4k and mips32 targets */
int mips_common_scan_delay(struct command_invocation *cmd);
int mips_common_cp0_command(struct command_invocation *cmd);

#endif	/*MIPS_COMMON_H*/
