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

uint32_t save_cpc_enable;
uint32_t save_gic_enable;

int mips_restore_cpc_enable_state (struct target *);
int mips_restore_gic_enable_state (struct target *);

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
//	uint32_t ejtag_ctrl;

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

uint32_t *mips_get_gic (struct target *target, int *retval)
{
	struct mips32_common *mips32 = target_to_mips32(target);
	struct mips_ejtag *ejtag_info = &mips32->ejtag_info;
	
	uint32_t *gcr_base;
	uint32_t *gic_base;
	uint32_t config3; /*	cp0 config - 16, 3 */
	uint32_t cmgcrbase; /*	cp0 config - 15, 3 */

	if ((*retval = mips32_pracc_cp0_read(ejtag_info, &config3, 16, 3))!= ERROR_OK)
		return ((uint32_t *)NULL);

	/* Read Config3 to determine if CMGCRBASE register is implemented */
	if ((config3 & 0x8) == 0){
		LOG_USER("CMGCRBASE not configured");
		return ((uint32_t *)NULL);
	}

	/* Read cmgcrbase config register */
	if ((*retval = mips32_pracc_cp0_read(ejtag_info, &cmgcrbase, 15, 3)) != ERROR_OK){
		LOG_DEBUG("target_read_u32 failed");
		return ((uint32_t *)NULL);
	}

	gcr_base = (uint32_t *)((cmgcrbase << 4) + 0xa0000000);

	*retval = target_read_u32(target, (uint32_t)(((uint32_t)gcr_base) +0x80), (uint32_t *) &gic_base);
	if (*retval != ERROR_OK) {
		LOG_DEBUG("target_read_u32 failed");
		return ((uint32_t *)NULL);
	}

	if (((uint32_t ) gic_base >> 24) != 0x1b){
		*retval = target_write_u32(target, (uint32_t)(((uint32_t)gcr_base) +0x80), 0x1BDC0000);
		if (*retval != ERROR_OK) {
			LOG_DEBUG("target_write_u32 failed");
			return ((uint32_t *)NULL);
		}

		*retval = target_read_u32(target, (uint32_t)(((uint32_t)gcr_base) +0x80), (uint32_t *) &gic_base);
		if (*retval != ERROR_OK) {
			LOG_DEBUG("target_read_u32 failed");
			return ((uint32_t *)NULL);
		}
	}

	/* Set CPC enable bit (0) */
	save_gic_enable = (uint32_t)gic_base & 1;
	gic_base = (uint32_t *)((uint32_t)gic_base | 0x1);
	*retval = target_write_u32(target, (uint32_t)(((uint32_t)gcr_base) +0x80), (uint32_t)gic_base);
	if (*retval != ERROR_OK) {
		LOG_DEBUG("target_write_u32 failed");
		return ((uint32_t *)NULL);
	}

	return ((uint32_t *)(((uint32_t)gic_base + 0xa0000000) & 0xffff0000));

}

int mips_restore_gic_enable_state (struct target *target)
{
	struct mips32_common *mips32 = target_to_mips32(target);
	struct mips_ejtag *ejtag_info = &mips32->ejtag_info;
	
	uint32_t *gcr_base;
	uint32_t temp;

	uint32_t config3; /*	cp0 config - 16, 3 */
	uint32_t cmgcrbase; /*	cp0 config - 15, 3 */

	int retval = -1;

	if ((retval = mips32_pracc_cp0_read(ejtag_info, &config3, 16, 3))!= ERROR_OK)
		return retval;

	/* Read Config3 to determine if CMGCRBASE register is implemented */
	if ((config3 & 0x8) == 0){
		LOG_USER("CMGCRBASE not configured");
		return ERROR_OK;
	}

	/* Read cmgcrbase config register */
	if ((retval = mips32_pracc_cp0_read(ejtag_info, &cmgcrbase, 15, 3)) != ERROR_OK)
		return retval;

	gcr_base = (uint32_t *)((cmgcrbase << 4) + 0xa0000000);

	retval = target_read_u32(target, (uint32_t)(((uint32_t)gcr_base) + 0x80), &temp);
	if (retval != ERROR_OK) {
		LOG_DEBUG("target_read_u32 failed");
		return retval;
	}

	/* Was CPC enable bit (0) set */
	if (save_gic_enable == 0)
		temp = temp & 0xfffffffe; 
	
	retval = target_write_u32(target, (uint32_t)(((uint32_t)gcr_base) +0x80), temp);
	if (retval != ERROR_OK) {
		LOG_DEBUG("target_write_u32 failed");
		return retval;
	}

	return ERROR_OK;
}

uint32_t * mips_get_cpc (struct target *target, int *retval)
{
	struct mips32_common *mips32 = target_to_mips32(target);
	struct mips_ejtag *ejtag_info = &mips32->ejtag_info;
	
	uint32_t *gcr_base;
	uint32_t *cpc_base;
	uint32_t config3; /*	cp0 config - 16, 3 */
	uint32_t cmgcrbase; /*	cp0 config - 15, 3 */

	if ((*retval = mips32_pracc_cp0_read(ejtag_info, &config3, 16, 3))!= ERROR_OK) {
		LOG_DEBUG("target_read_u32 failed");
		return ((uint32_t *)NULL);
	}

	/* Read Config3 to determine if CMGCRBASE register is implemented */
	if ((config3 & 0x8) == 0){
		LOG_USER("CMGCRBASE not configured");
		return ((uint32_t *)NULL);
	}

	/* Read cmgcrbase config register */
	if ((*retval = mips32_pracc_cp0_read(ejtag_info, &cmgcrbase, 15, 3)) != ERROR_OK){
		LOG_DEBUG("target_read_u32 failed");
		return ((uint32_t *)NULL);
	}

	gcr_base = (uint32_t *)((cmgcrbase << 4) + 0xa0000000);

	/* get CPC base */
	*retval = target_read_u32(target, (uint32_t)(((uint32_t)gcr_base) +0x88), (uint32_t *)&cpc_base);
	if (*retval != ERROR_OK) {
		LOG_DEBUG("target_read_u32 failed");
		return ((uint32_t *)NULL);
	}

	/* Has CPC address been initialized */
	if (((uint32_t )cpc_base >> 24) != 0x1b){ 
		*retval = target_write_u32(target, (uint32_t)(((uint32_t)gcr_base) +0x88), 0x1BDE0000);
		if (*retval != ERROR_OK) {
			LOG_DEBUG("target_write_u32 failed");
			return ((uint32_t *)NULL);
		}

		*retval = target_read_u32(target, (uint32_t)(((uint32_t)gcr_base) +0x88), (uint32_t *)&cpc_base);
		if (*retval != ERROR_OK) {
			LOG_DEBUG("target_read_u32 failed");
			return ((uint32_t *)NULL);
		}
	}

	/* Set CPC enable bit (0) */
	save_cpc_enable = (uint32_t)cpc_base & 1;
	cpc_base = (uint32_t *)((uint32_t)cpc_base | 0x1);
	*retval = target_write_u32(target, (uint32_t)(((uint32_t)gcr_base) +0x88), (uint32_t)cpc_base);
	if (*retval != ERROR_OK) {
		LOG_DEBUG("target_write_u32 failed");
		return ((uint32_t *)NULL);
	}

	return ((uint32_t *)(((uint32_t)cpc_base + 0xa0000000) & 0xffff0000));
}


int mips_restore_cpc_enable_state (struct target *target)
{
	struct mips32_common *mips32 = target_to_mips32(target);
	struct mips_ejtag *ejtag_info = &mips32->ejtag_info;
	
	uint32_t *gcr_base;
	uint32_t temp;

	uint32_t config3; /*	cp0 config - 16, 3 */
	uint32_t cmgcrbase; /*	cp0 config - 15, 3 */

	int retval = -1;

	if ((retval = mips32_pracc_cp0_read(ejtag_info, &config3, 16, 3))!= ERROR_OK)
		return retval;

	/* Read Config3 to determine if CMGCRBASE register is implemented */
	if ((config3 & 0x8) == 0){
		LOG_USER("CMGCRBASE not configured");
		return ERROR_OK;
	}

	/* Read cmgcrbase config register */
	if ((retval = mips32_pracc_cp0_read(ejtag_info, &cmgcrbase, 15, 3)) != ERROR_OK)
		return retval;

	gcr_base = (uint32_t *)((cmgcrbase << 4) + 0xa0000000);

	retval = target_read_u32(target, (uint32_t)(((uint32_t)gcr_base) + 0x88), &temp);
	if (retval != ERROR_OK) {
		LOG_DEBUG("target_read_u32 failed");
		return retval;
	}

	/* Was CPC enable bit (0) set */
	if (save_cpc_enable == 0)
		temp = temp & 0xfffffffe; 
	
	retval = target_write_u32(target, (uint32_t)(((uint32_t)gcr_base) +0x88), temp);
	if (retval != ERROR_OK) {
		LOG_DEBUG("target_write_u32 failed");
		return retval;
	}

	return ERROR_OK;
}

COMMAND_HANDLER(mips_ia_handle_cmdall_command)
{
	struct target *target = get_current_target(CMD_CTX);

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

/*                                                                                     */
/* Command is used to dump the Coherency Manager Global Configuration Registers.       */
/* The Global Configuration Registers (GCR) are a set of memory-mapped registers that  */
/* are used to configure and control various aspects of the Coherence Manager and the  */
/* coherence scheme.                                                                   */
/* See section 8.3.1 of Mips32 interAptiv Multiprocessing System Programmer's Guide    */
/* Document available at: http://wiki.prplfoundation.org/wiki/MIPS_documentation       */
/*                                                                                     */
COMMAND_HANDLER(mips_dump_gcr_command)
{
	struct target *target = get_current_target(CMD_CTX);
	struct mips32_common *mips32 = target_to_mips32(target);
	struct mips_ejtag *ejtag_info = &mips32->ejtag_info;
	
	uint32_t *gcr_base;
	uint32_t *gic_base;
	uint32_t *cpc_base;
	uint32_t temp;

	uint32_t config3; /*	cp0 config - 16, 3 */
	uint32_t cmgcrbase; /*	cp0 config - 15, 3 */

	int retval = -1;

	if (CMD_ARGC >= 1){
		LOG_DEBUG("ERROR_COMMAND_SYNTAX_ERROR");
		return ERROR_COMMAND_SYNTAX_ERROR;
	}

	if (target->state != TARGET_HALTED) {
		command_print(CMD_CTX, "target must be stopped for \"%s\" command", CMD_NAME);
		return ERROR_OK;
	}

	if ((retval = mips32_pracc_cp0_read(ejtag_info, &config3, 16, 3))!= ERROR_OK)
		return retval;

	/* May need to read Config3 to determine if CMGCRBASE register is implemented */
	if ((config3 & 0x8) == 0){
		LOG_USER("CMGCRBASE not configured");
		return ERROR_OK;
	}

	/* Read cmgcrbase config register */
	if ((retval = mips32_pracc_cp0_read(ejtag_info, &cmgcrbase, 15, 3)) != ERROR_OK)
		return retval;

	gcr_base = (uint32_t *)((cmgcrbase << 4) + 0xa0000000);

	retval = target_read_u32(target, (uint32_t)gcr_base, &temp);
	if (retval != ERROR_OK) {
		LOG_DEBUG("target_read_u32 failed");
		return retval;
	}

	LOG_USER ("gcr_config  = 0x%8.8x", temp);

	retval = target_read_u32(target, (uint32_t)(((uint32_t)gcr_base) + 0x8), &temp);
	if (retval != ERROR_OK) {
		LOG_DEBUG("target_read_u32 failed");
		return retval;
	}

	LOG_USER ("gcr_base    = 0x%8.8x", temp);

	retval = target_read_u32(target, (uint32_t)(((uint32_t)gcr_base) + 0x10), &temp);
	if (retval != ERROR_OK) {
		LOG_DEBUG("target_read_u32 failed");
		return retval;
	}

	LOG_USER ("gcr_control = 0x%8.8x", temp);

	retval = target_read_u32(target, (uint32_t)(((uint32_t)gcr_base) + 0x20), &temp);
	if (retval != ERROR_OK) {
		LOG_DEBUG("target_read_u32 failed");
		return retval;
	}

	LOG_USER ("gcr_access  = 0x%8.8x", temp);

	retval = target_read_u32(target, (uint32_t)(((uint32_t)gcr_base) + 0x30), &temp);
	if (retval != ERROR_OK) {
		LOG_DEBUG("target_read_u32 failed");
		return retval;
	}

	LOG_USER ("gcr_rev     = 0x%8.8x", temp);

	gic_base = mips_get_gic (target, &retval);
	if (retval != ERROR_OK)
		return retval;

	cpc_base = mips_get_cpc (target, &retval);
	if (retval != ERROR_OK)
		return retval;

	LOG_USER ("gic_base    = 0x%8.8x", (uint32_t)gic_base);
	mips_restore_gic_enable_state(target);
	LOG_USER ("cpc_base    = 0x%8.8x", (uint32_t)cpc_base);
	mips_restore_cpc_enable_state(target);
	return ERROR_OK;
}

/*                                                                                     */
/* Command is used to dump the Coherency Manager Global Configuration Registers.       */
/* The Global Configuration Registers (GCR) are a set of memory-mapped registers that  */
/* are used to configure and control various aspects of the Coherence Manager and the  */
/* coherence scheme.                                                                   */
/* See section 8.3.1 of Mips32 interAptiv Multiprocessing System Programmer's Guide    */
/* Document available at: http://wiki.prplfoundation.org/wiki/MIPS_documentation       */
/*                                                                                     */
COMMAND_HANDLER(mips_dump_lcr_command)
{
	struct target *target = get_current_target(CMD_CTX);
	struct mips32_common *mips32 = target_to_mips32(target);
	struct mips_ejtag *ejtag_info = &mips32->ejtag_info;
	
	if (CMD_ARGC > 1){
		LOG_DEBUG("ERROR_COMMAND_SYNTAX_ERROR");
		return ERROR_COMMAND_SYNTAX_ERROR;
	}

	if (target->state != TARGET_HALTED) {
		command_print(CMD_CTX, "target must be stopped for \"%s\" command", CMD_NAME);
		return ERROR_OK;
	}

	uint32_t core = 0;
	uint32_t offset = 0x2000;

	if (CMD_ARGC != 0){
		COMMAND_PARSE_NUMBER(u32, CMD_ARGV[0], core);
		offset = 0x4000;
	}
		
	uint32_t config3; /*	cp0 config - 16, 3 */

	int retval = -1;

	/* Read Config3 */
	if ((retval = mips32_pracc_cp0_read(ejtag_info, &config3, 16, 3))!= ERROR_OK)
		return retval;

	/* Determine if CMGCRBASE register is implemented */
	if ((config3 & 0x8) == 0){
		LOG_USER("CMGCRBASE not configured");
		return ERROR_OK;
	}

	uint32_t cmgcrbase; /*	cp0 config - 15, 3 */

	/* Read cmgcrbase config register */
	if ((retval = mips32_pracc_cp0_read(ejtag_info, &cmgcrbase, 15, 3)) != ERROR_OK)
		return retval;

	uint32_t *gcr_base;
	uint32_t temp;

	gcr_base = (uint32_t *)((cmgcrbase << 4) + 0xa0000000) ;

	/* GCR CL (gcr_base+0) Base address */
	if (core != 0){
		retval = target_write_u32(target, (uint32_t)(((uint32_t)gcr_base) + 0x2018), (uint32_t)(core << 16));
		if (retval != ERROR_OK) {
			LOG_DEBUG("target_read_u32 failed");
			return retval;
		}

		LOG_USER ("GCR CL Other = 0x%8.8x", (uint32_t)(((uint32_t)gcr_base) + offset));
	}
	else
		LOG_USER ("GCR CL       = 0x%8.8x", (uint32_t)(((uint32_t)gcr_base) + offset));

	/* cl_coherence (gcr_base+0x2000/0x4000 + 0x8 - Core-Local Coherence Control. */
	retval = target_read_u32(target, (uint32_t)(((uint32_t)gcr_base) + offset + 0x08), &temp);
	if (retval != ERROR_OK) {
		LOG_DEBUG("target_read_u32 failed");
		return retval;
	}

	LOG_USER ("cl_coherence = 0x%8.8x", temp);

	/* cl_config (gcr_base+0x2000/0x4000 + 0x10) - Core-Local Configuration */
	retval = target_read_u32(target, (uint32_t)(((uint32_t)gcr_base) + offset + 0x10), &temp);
	if (retval != ERROR_OK) {
		LOG_DEBUG("target_read_u32 failed");
		return retval;
	}

	LOG_USER ("cl_config    = 0x%8.8x", temp);

	/* cl_other (gcr_base+0x2000/0x4000 + 0x18) - Core-Other Addressing */
	retval = target_read_u32(target, (uint32_t)(((uint32_t)gcr_base) + offset + 0x18), &temp);
	if (retval != ERROR_OK) {
		LOG_DEBUG("target_read_u32 failed");
		return retval;
	}

	LOG_USER ("cl_other     = 0x%8.8x", temp);

	/* cl_resetbase (gcr_base+0x2000/0x4000 + 0x20) - Reset Exception Base for the local core. */
	retval = target_read_u32(target, (uint32_t)(((uint32_t)gcr_base) + offset + 0x20), &temp);
	if (retval != ERROR_OK) {
		LOG_DEBUG("target_read_u32 failed");
		return retval;
	}

	LOG_USER ("cl_resetbase = 0x%8.8x", temp);

	/* cl_id (gcr_base+0x2000/0x4000 + 0x28) - Indicates the ID number of the local core. */
	retval = target_read_u32(target, (uint32_t)(((uint32_t)gcr_base) + offset + 0x28), &temp);
	if (retval != ERROR_OK) {
		LOG_DEBUG("target_read_u32 failed");
		return retval;
	}

	LOG_USER ("cl_id        = 0x%8.8x", temp);

	return ERROR_OK;
}

/*                                                                                     */
/* Command is used to dump the Cluster Power Controller Global Control Block Registers */
/* See section 7.3.2 of Mips32 interAptiv Multiprocessing System Programmer's Guide    */
/* Document available at: http://wiki.prplfoundation.org/wiki/MIPS_documentation       */
/*                                                                                     */
COMMAND_HANDLER(mips_dump_cpc_command)
{
	struct target *target = get_current_target(CMD_CTX);
	struct mips32_common *mips32 = target_to_mips32(target);
	struct mips_ejtag *ejtag_info = &mips32->ejtag_info;
	
	if (CMD_ARGC >= 1){
		LOG_DEBUG("ERROR_COMMAND_SYNTAX_ERROR");
		return ERROR_COMMAND_SYNTAX_ERROR;
	}

	if (target->state != TARGET_HALTED) {
		command_print(CMD_CTX, "target must be stopped for \"%s\" command", CMD_NAME);
		return ERROR_OK;
	}

	uint32_t *cpc_base = 0;
	uint32_t temp;

	uint32_t config3; /*	cp0 config - 16, 3 */
	uint32_t cmgcrbase; /*	cp0 config - 15, 3 */

	int retval = -1;

	if ((retval = mips32_pracc_cp0_read(ejtag_info, &config3, 16, 3))!= ERROR_OK)
		return retval;

	/* Read Config3 to determine if CMGCRBASE register is implemented */
	if ((config3 & 0x8) == 0){
		LOG_USER("CMGCRBASE not configured");
		return ERROR_OK;
	}

	/* Read cmgcrbase config register */
	if ((retval = mips32_pracc_cp0_read(ejtag_info, &cmgcrbase, 15, 3)) != ERROR_OK)
		return retval;

	/* get CPC base */
	cpc_base = mips_get_cpc (target, &retval);
	if (retval != ERROR_OK)
		return retval;

	LOG_USER ("CPC_BASE         = 0x%8.8x", (uint32_t)cpc_base);

	/* CPC_ACCESS_REG (cpc_base + 0) - Controls which cores can modify the CPC Registers.*/
	retval = target_read_u32(target, (uint32_t)(uint32_t)cpc_base, &temp);
	if (retval != ERROR_OK) {
		LOG_DEBUG("target_read_u32 failed");
		goto exit;
	}

	LOG_USER ("CPC_ACCESS_REG   = 0x%x", temp);

	/* CPC_SEQDEL_REG (cpc_base + 8) - Time between microsteps of a CPC domain */
	/*                                 sequencer in CPC clock cycles.          */
	retval = target_read_u32(target, (uint32_t)(((uint32_t)cpc_base) + 0x8), &temp);
	if (retval != ERROR_OK) {
		LOG_DEBUG("target_read_u32 failed");
		goto exit;
	}

	LOG_USER ("CPC_SEQDEL_REG   = 0x%x", temp);

	/* CPC_RAIL_REG (cpc_base + 0x10) - Rail power-up timer to delay CPS            */
    /*                                  sequencer progress until the gated rail has */
    /*                                  stabilized.                                 */
	retval = target_read_u32(target, (uint32_t)(((uint32_t)cpc_base) + 0x10), &temp);
	if (retval != ERROR_OK) {
		LOG_DEBUG("target_read_u32 failed");
		goto exit;
	}

	LOG_USER ("CPC_RAIL_REG     = 0x%x", temp);

	/* CPC_RESETLEN_REG (cpc_base + 0x18) - Duration of any domain reset sequence. */
	retval = target_read_u32(target, (uint32_t)(((uint32_t)cpc_base) + 0x18), &temp);
	if (retval != ERROR_OK) {
		LOG_DEBUG("target_read_u32 failed");
		goto exit;
	}

	LOG_USER ("CPC_RESETLEN_REG = 0x%x", temp);

	/* CPC_REVISION_REG (cpc_base + 0x20) - RTL Revision of CPC */
	retval = target_read_u32(target, (uint32_t)(((uint32_t)cpc_base) + 0x20), &temp);
	if (retval != ERROR_OK) {
		LOG_DEBUG("target_read_u32 failed");
		goto exit;
	}

	LOG_USER ("CPC_REVISION_REG = 0x%x", temp);

exit:
	mips_restore_cpc_enable_state(target);
	return ERROR_OK;
}

/*                                                                                      */
/* Command is used to dump the Cluster Power Controller Local and Core-Other Control    */
/* Block. See section 7.3.4 of Mips32 interAptiv Multiprocessing System Programmer's    */
/* Guide. Document available at: http://wiki.prplfoundation.org/wiki/MIPS_documentation */
/*                                                                                      */
COMMAND_HANDLER(mips_dump_cpc_lcr_command)
{
	struct target *target = get_current_target(CMD_CTX);
	struct mips32_common *mips32 = target_to_mips32(target);
	struct mips_ejtag *ejtag_info = &mips32->ejtag_info;
	uint32_t cpc_stat_conf;

	if (CMD_ARGC > 1){
		LOG_DEBUG("ERROR_COMMAND_SYNTAX_ERROR");
		return ERROR_COMMAND_SYNTAX_ERROR;
	}

	if (target->state != TARGET_HALTED) {
		command_print(CMD_CTX, "target must be stopped for \"%s\" command", CMD_NAME);
		return ERROR_OK;
	}

	uint32_t core = 0;
	uint32_t offset = 0x2000;

	if (CMD_ARGC != 0){
		COMMAND_PARSE_NUMBER(u32, CMD_ARGV[0], core);
		offset = 0x4000;
	}

	uint32_t config3; /*	cp0 config - 16, 3 */
	uint32_t cmgcrbase; /*	cp0 config - 15, 3 */

	int retval = -1;

	if ((retval = mips32_pracc_cp0_read(ejtag_info, &config3, 16, 3))!= ERROR_OK)
		return retval;

	/* May need to read Config3 to determine if CMGCRBASE register is implemented */
	if ((config3 & 0x8) == 0){
		LOG_USER("CMGCRBASE not configured");
		return ERROR_OK;
	}

	/* Read cmgcrbase config register */
	if ((retval = mips32_pracc_cp0_read(ejtag_info, &cmgcrbase, 15, 3)) != ERROR_OK)
		return retval;

	uint32_t *cpc_base;
	uint32_t temp;

	/* get CPC base */
	cpc_base = mips_get_cpc (target, &retval);
	if (retval != ERROR_OK)
		return retval;

	if (core != 0){
		LOG_USER ("CPC CL Other         = 0x%8.8x", (uint32_t)((uint32_t)cpc_base + offset));
		retval = target_write_u32(target, (uint32_t) (uint32_t)(((uint32_t)cpc_base) + 0x2010), (uint32_t)(core << 16));
		if (retval != ERROR_OK) {
			LOG_DEBUG("target_read_u32 failed");
			return retval;
		}
	}
	else
		LOG_USER ("CPC CL               = 0x%8.8x", (uint32_t)((uint32_t)cpc_base + offset));

	/* CPC_CL_CMD_REG (cpc_base+0x2000/0x4000) - Places a new CPC domain state command */
	/* into this individual domain sequencer. This register is not available    */
	/* within the CM sequencer. Writes to the CM CMD register are ignored while */
	/* reads will return zero.                                                  */
	retval = target_read_u32(target, (uint32_t)(((uint32_t)cpc_base) + offset), &temp);
	if (retval != ERROR_OK) {
		LOG_DEBUG("target_read_u32 failed");
		goto exit;
	}

	LOG_USER ("CPC_CL_CMD_REG       = 0x%x", temp);

	retval = target_read_u32(target, (uint32_t)(((uint32_t)cpc_base) + offset + 0x10), &temp);
	if (retval != ERROR_OK) {
		LOG_DEBUG("target_read_u32 failed");
		goto exit;
	}

	LOG_USER ("CPC_CL_OTHER_REG     = 0x%x", temp);
	
	retval = target_read_u32(target, (uint32_t)(((uint32_t)cpc_base) + offset + 0x8), &cpc_stat_conf);
	if (retval != ERROR_OK) {
		LOG_DEBUG("target_read_u32 failed");
		goto exit;
	}

    LOG_USER ("\nCPC_CL_STAT_CONF_REG = 0x%8.8x", cpc_stat_conf);

	LOG_USER(" Decode of CPC_CL_STAT_CONF_REG");
	LOG_USER(" ------------------------------");

	/* Decode Seq State */
	// int seq_state;
	char text[20]={0};
	
	LOG_USER("      PWRUP_EVENT (23) = 0x%x", (cpc_stat_conf >> 23) & 1);

	switch ((cpc_stat_conf >> 19) & 0xf) {
		case 0:
			strcpy (text,"D0 - PwrDwn");
			break;
		case 1:
			strcpy (text,"U0 - VddOK");
			break;
		case 2:
			strcpy (text,"U1 - UpDelay");
			break;
		case 3:
			strcpy (text,"U2 - UClkOff");
			break;
		case 4:
			strcpy (text,"U3 - Reset");
			break;
		case 5:
			strcpy (text,"U4 - ResetDly");
			break;
		case 6:
			strcpy (text,"U5 - nonCoherent");
			break;
		case 7:
			strcpy (text,"U6 - Coherent");
			break;
		case 8:
			strcpy (text,"D1 - Isolate");
			break;
		case 9:
			strcpy (text,"D3 - ClrBus");
			break;
		case 10:
			strcpy (text,"D2 - DClkOff");
			break;
		default:
		{
			strcpy (text,"Invalid");
		}
	}

	LOG_USER("     SEQ_STATE (22:19) = 0x%x (%s)", ((cpc_stat_conf >> 19) & 0xf), text);
	LOG_USER("      CLKGAT_IMPL (17) = 0x%x",((cpc_stat_conf >> 17) & 0x1));
	LOG_USER("       PWRDN_IMPL (16) = 0x%x", ((cpc_stat_conf >> 16) & 0x1));
	LOG_USER("      EJTAG_PROBE (15) = 0x%x", ((cpc_stat_conf >> 15) & 0x1));
	LOG_USER("     PWUP_POLICY (9:8) = 0x%x", ((cpc_stat_conf >> 8) & 0x3));
	LOG_USER("       IO_TRFFC_EN (4) = 0x%x", ((cpc_stat_conf >> 4) & 0x1));

	switch (cpc_stat_conf & 0xf) {
		case 0:
			strcpy (text,"none");
			break;
		case 1:
			strcpy (text,"ClockOff");
			break;
		case 2:
			strcpy (text,"PwrDown");
			break;
		case 3:
			strcpy (text,"PwrUp");
			break;
		case 4:
			strcpy (text,"Reset");
			break;
		default:
		{
			strcpy (text,"reserved");
		}
    }

	LOG_USER("             CMD (3:0) = 0x%x (%s)", (cpc_stat_conf & 0xf), text);

exit:	
	mips_restore_cpc_enable_state(target);
	return ERROR_OK;
}

COMMAND_HANDLER(mips_dump_gic_shared_command)
{
	struct target *target = get_current_target(CMD_CTX);
	struct mips32_common *mips32 = target_to_mips32(target);
	struct mips_ejtag *ejtag_info = &mips32->ejtag_info;
	
	if (CMD_ARGC >= 1){
		LOG_DEBUG("ERROR_COMMAND_SYNTAX_ERROR");
		return ERROR_COMMAND_SYNTAX_ERROR;
	}

	if (target->state != TARGET_HALTED) {
		command_print(CMD_CTX, "target must be stopped for \"%s\" command", CMD_NAME);
		return ERROR_OK;
	}

	uint32_t *gic_base;
	uint32_t temp;

	uint32_t config3; /*	cp0 config - 16, 3 */
	uint32_t cmgcrbase; /*	cp0 config - 15, 3 */

	int retval = ERROR_OK;

	/* Read Config3 register */
	if ((retval = mips32_pracc_cp0_read(ejtag_info, &config3, 16, 3))!= ERROR_OK)
		return retval;

	/* Examine Config3 to determine if CMGCRBASE register is implemented */
	if ((config3 & 0x8) == 0){
		LOG_USER("CMGCRBASE not configured");
		return ERROR_OK;
	}

	/* Read cmgcrbase config register */
	if ((retval = mips32_pracc_cp0_read(ejtag_info, &cmgcrbase, 15, 3)) != ERROR_OK)
		return retval;

	gic_base = mips_get_gic (target, &retval);
	if (retval != ERROR_OK) {
		return retval;
	}

	LOG_USER ("GIC BASE = 0x%8.8x", (uint32_t)((uint32_t)gic_base));

	retval = target_read_u32(target, (uint32_t)gic_base, &temp);
	if (retval != ERROR_OK) {
		LOG_DEBUG("target_read_u32 failed");
		goto exit;
	}

	LOG_USER ("GIC_SH_CONFIG_REG    = 0x%8.8x", temp);

	retval = target_read_u32(target, (uint32_t)(((uint32_t)gic_base) + 0x10), &temp);
	if (retval != ERROR_OK) {
		LOG_DEBUG("target_read_u32 failed");
		goto exit;
	}

	LOG_USER ("GIC_SH_CounterLo_REG = 0x%8.8x", temp);

	retval = target_read_u32(target, (uint32_t)(((uint32_t)gic_base) + 0x14), &temp);
	if (retval != ERROR_OK) {
		LOG_DEBUG("target_read_u32 failed");
		goto exit;
	}

	LOG_USER ("GIC_SH_CounterHi_REG = 0x%8.8x", temp);

	retval = target_read_u32(target, (uint32_t)(((uint32_t)gic_base) + 0x20), &temp);
	if (retval != ERROR_OK) {
		LOG_DEBUG("target_read_u32 failed");
		goto exit;
	}

	LOG_USER ("GIC_RevisionID_REG   = 0x%8.8x", temp);

exit:
	mips_restore_gic_enable_state(target);
	return retval;;
}

COMMAND_HANDLER(mips_dump_gic_local_command)
{
	struct target *target = get_current_target(CMD_CTX);
	struct mips32_common *mips32 = target_to_mips32(target);
	struct mips_ejtag *ejtag_info = &mips32->ejtag_info;
	
	if (CMD_ARGC >= 1){
		LOG_DEBUG("ERROR_COMMAND_SYNTAX_ERROR");
		return ERROR_COMMAND_SYNTAX_ERROR;
	}

	if (target->state != TARGET_HALTED) {
		command_print(CMD_CTX, "target must be stopped for \"%s\" command", CMD_NAME);
		return ERROR_OK;
	}

	uint32_t *gic_base;
	uint32_t *gic_vpe_local_base;
	uint32_t temp;

	uint32_t config3; /*	cp0 config - 16, 3 */
	uint32_t cmgcrbase; /*	cp0 config - 15, 3 */

	int retval = ERROR_OK;

	if ((retval = mips32_pracc_cp0_read(ejtag_info, &config3, 16, 3))!= ERROR_OK)
		return retval;

	/* May need to read Config3 to determine if CMGCRBASE register is implemented */
	if ((config3 & 0x8) == 0){
		LOG_USER("CMGCRBASE not configured");
		return ERROR_OK;
	}

	/* Read cmgcrbase config register */
	if ((retval = mips32_pracc_cp0_read(ejtag_info, &cmgcrbase, 15, 3)) != ERROR_OK)
		return retval;

	gic_base = mips_get_gic (target, &retval);
	if (retval != ERROR_OK)
		return retval;

	gic_vpe_local_base = (uint32_t *)(((uint32_t)gic_base + 0x8000));
	LOG_USER ("GIC_VPE_LOCAL_BASE      = 0x%8.8x", (uint32_t)gic_vpe_local_base);

	retval = target_read_u32(target, (uint32_t)(((uint32_t)gic_vpe_local_base) + 0x80), &temp);
	if (retval != ERROR_OK) {
		LOG_DEBUG("target_read_u32 failed");
		goto exit;
	}

	LOG_USER ("GIC_VPEi_OTHER_ADDR     = 0x%8.8x", temp);

	retval = target_read_u32(target, (uint32_t)(((uint32_t)gic_vpe_local_base) + 0x88), &temp);
	if (retval != ERROR_OK) {
		LOG_DEBUG("target_read_u32 failed");
		goto exit;
	}

	LOG_USER ("GIC_VPEi_IDENT          = 0x%8.8x", temp);

	retval = target_read_u32(target, (uint32_t)(((uint32_t)gic_vpe_local_base) + 0x90), &temp);
	if (retval != ERROR_OK) {
		LOG_DEBUG("target_read_u32 failed");
		goto exit;
	}

	LOG_USER ("GIC_VPEi_WD_CONFIG0     = 0x%8.8x", temp);

	retval = target_read_u32(target, (uint32_t)(((uint32_t)gic_vpe_local_base) + 0x94), &temp);
	if (retval != ERROR_OK) {
		LOG_DEBUG("target_read_u32 failed");
		goto exit;
	}

	LOG_USER ("GIC_VPEi_WD_COUNT0      = 0x%8.8x", temp);

	retval = target_read_u32(target, (uint32_t)(((uint32_t)gic_vpe_local_base) + 0x3000), &temp);
	if (retval != ERROR_OK) {
		LOG_DEBUG("target_read_u32 failed");
		goto exit;
	}

	LOG_USER ("GIC_VPEi_DINT           = 0x%x", temp);

	retval = target_read_u32(target, (uint32_t)(((uint32_t)gic_vpe_local_base) + 0x3080), &temp);
	if (retval != ERROR_OK) {
		LOG_DEBUG("target_read_u32 failed");
		goto exit;
	}

	LOG_USER ("GIC_VPEi_LOCAL_DEBUG_GR = 0x%8.8x", temp);

	retval = target_read_u32(target, (uint32_t)(((uint32_t)gic_vpe_local_base) + 0x88), &temp);
	if (retval != ERROR_OK) {
		LOG_DEBUG("target_read_u32 failed");
	}

exit:
	mips_restore_gic_enable_state(target);
	return retval;
}

static const struct command_registration mips_iAptiv_exec_command_handlers[] = {
	{
		.name = "cmdall",
		.handler = mips_ia_handle_cmdall_command,
		.mode = COMMAND_EXEC,
		.usage = "halt",
		.help = "cmdall halt command will halt all cores",
	},
	{
		.name = "dump_gcr",
		.handler = mips_dump_gcr_command,
		.mode = COMMAND_EXEC,
		.usage = "dump_gcr",
		.help = "dump GCR registers",
	},
	{
		.name = "dump_lcr",
		.handler = mips_dump_lcr_command,
		.mode = COMMAND_EXEC,
		.usage = "dump_lcr [core]",
		.help = "dump GCR local registers",
	},
	{
		.name = "dump_gic_shared",
		.handler = mips_dump_gic_shared_command,
		.mode = COMMAND_EXEC,
		.usage = "dump_gic_shared",
		.help = "dump Global Interrupt Controller Shared",
	},
	{
		.name = "dump_gic_local",
		.handler = mips_dump_gic_local_command,
		.mode = COMMAND_EXEC,
		.usage = "dump_gic_local",
		.help = "dump Global Interrupt Controller Local Registers",
	},
	{
		.name = "dump_cpc",
		.handler = mips_dump_cpc_command,
		.mode = COMMAND_EXEC,
		.usage = "dump_cpc",
		.help = "Dump Cluster Power Controller",
	},
	{
		.name = "dump_cpc_lcr",
		.handler = mips_dump_cpc_lcr_command,
		.mode = COMMAND_ANY,
		.usage = "dump_cpc_lcr [core]",
		.help = "Dump Cluster Power Controller Local/Other Command Registers",
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
