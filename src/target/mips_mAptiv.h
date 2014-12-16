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

#ifndef MIPS_MAPTIV_H
#define MIPS_MAPTIV_H

struct target;

#define MIPSMAPTIV_COMMON_MAGIC	0xB321B321

struct mips_mAptiv_common {
	uint32_t common_magic;
	bool is_pic32mx;
	struct mips32_common mips32;
};

inline struct mips_mAptiv_common *target_to_mAptiv(struct target *target)
{
	return container_of(target->arch_info,
			struct mips_mAptiv_common, mips32);
}

extern const struct command_registration mips_mAptiv_command_handlers[];

#endif	/*MIPS_MAPTIV_H*/
