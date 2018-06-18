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

#ifndef MIPS_IAPTIV_H
#define MIPS_IAPTIV_H

struct target;

#define MIPS_IAPTIV_COMMON_MAGIC	0xB321B321

#define MIPS_IA_NUMCP0REGS 95

static const struct {
	unsigned reg;
	unsigned sel;
	const char *name;
} mips32_iA_cp0_regs[MIPS_IA_NUMCP0REGS] = {
  { 0, 0, "index"},
  { 0, 1, "mvpcontrol"},
  { 0, 2, "mvpconf0"},
  { 0, 3, "mvpconf1"},
  { 1, 0, "Random"},
  { 1, 1, "VPEControl"},
  { 1, 2, "VPEConf0"},
  { 1, 3, "VPEConf1"},
  { 1, 4, "YQMask"},
  { 1, 5, "VPESchedule"},
  { 1, 6, "VPEScheFBack"},
  { 1, 7, "VPEOpt"},
  { 2, 0, "EntryLo0"},
  { 2, 1, "TCStatus"},
  { 2, 2, "TCBind"},
  { 2, 3, "TCRestart"},
  { 2, 4, "TCHalt"},
  { 2, 5, "TCContext"},
  { 2, 6, "TCSchedule"},
  { 2, 7, "TCScheFBack"},
  { 3, 0, "EntryLo1"},
  { 3, 7, "TCOpt"},
  { 4, 0, "Context"},
  { 4, 2, "userlocal"},
  { 5, 0, "PageMask"},
  { 5, 2, "SegCtl0"},
  { 5, 3, "SegCtl1"},
  { 5, 4, "SegCtl2"},
  { 6, 0, "Wired"},
  { 6, 1, "SRSConf0"},
  { 6, 2, "SRSConf1"},
  { 6, 3, "SRSConf2"},
  { 6, 4, "SRSConf3"},
  { 6, 5, "SRSConf4"},
  { 7, 0, "hwrena"},
  { 8, 0, "badvaddr"},
  { 9, 0, "Count"},
  { 10, 0, "EntryHi"},
  { 11, 0, "compare"},
  { 11, 4, "GuestCtl0Ext"},
  { 12, 0, "status"},
  { 12, 1, "intctl"},
  { 12, 2, "srsctl"},
  { 12, 3, "SRSMap2"},
  { 13, 0, "cause"},
  { 14, 0, "epc"},
  { 14, 2, "nestedepc"},
  { 15, 0, "prid"},
  { 15, 1, "ebase"},
  { 15, 2, "cdmmbase"},
  { 15, 3, "CMGCRBase"},
  { 16, 0, "config"},
  { 16, 1, "config1"},
  { 16, 2, "config2"},
  { 16, 3, "config3"},
  { 16, 4, "config4"},
  { 16, 5, "config5"},
  { 16, 7, "config7"},
  { 17, 0, "lladdr"},
  { 18, 0, "WatchLo0"},
  { 18, 1, "WatchLo1"},
  { 18, 2, "WatchLo2"},
  { 18, 3, "WatchLo3"},
  { 19, 0, "WatchHi0"},
  { 19, 1, "WatchHi1"},
  { 19, 2, "WatchHi2"},
  { 19, 3, "WatchHi3"},
  { 23, 0, "debug"},
  { 23, 6, "Debug2"},
  { 23, 1, "tracecontrol"},
  { 23, 2, "tracecontrol2"},
  { 23, 3, "usertracedata1"},
  { 23, 4, "TraceIBPC"},
  { 23, 5, "TraceDBPC"},
  { 24, 0, "depc"},
  { 24, 2, "TraceControl3"},
  { 24, 3, "usertracedata2"},
  { 25, 0, "perfctl0"},
  { 25, 1, "perfcnt0"},
  { 25, 2, "perfctl1"},
  { 25, 3, "perfcnt1"},
  { 26, 0, "errctl"},
  { 27, 0, "CacheErr"},
  { 28, 0, "ITagLo"},
  { 28, 1, "IDataLo"},
  { 28, 2, "DTagLo"},
  { 28, 3, "DDataLo"},
  { 28, 4, "L23TagLo"},
  { 28, 5, "L23DataLo"},
  { 29, 1, "IDataHi"},
  { 29, 2, "DTagHi"},
  { 29, 5, "L23DataHi"},
  { 30, 0, "errorepc"},
  { 31, 0, "desave"},
};

struct mips_iAptiv_common {
	uint32_t common_magic;
	bool is_pic32;
	struct mips32_common mips32;
};

inline struct mips_iAptiv_common *target_to_iAptiv(struct target *target)
{
	return container_of(target->arch_info,
			struct mips_iAptiv_common, mips32);
}

extern const struct command_registration mips_iAptiv_command_handlers[];

#endif	/*MIPS_IAPTIV_H*/
