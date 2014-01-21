/***************************************************************************
 *   Copyright (C) 2008 by John McCarthy                                   *
 *   jgmcc@magma.ca                                                        *
 *                                                                         *
 *   Copyright (C) 2008 by Spencer Oliver                                  *
 *   spen@spen-soft.co.uk                                                  *
 *                                                                         *
 *   Copyright (C) 2008 by David T.L. Wong                                 *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.           *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mips32_dmaacc.h"
#include <helper/time_support.h>

/*
 * The following logic shamelessly cloned from HairyDairyMaid's wrt54g_debrick
 * to support the Broadcom BCM5352 SoC in the Linksys WRT54GL wireless router
 * (and any others that support EJTAG DMA transfers).
 * Note: This only supports memory read/write. Since the BCM5352 doesn't
 * appear to support PRACC accesses, all debug functions except halt
 * do not work.  Still, this does allow erasing/writing flash as well as
 * displaying/modifying memory and memory mapped registers.
 */

static int ejtag_dma_dstrt_poll(struct mips_ejtag *ejtag_info)
{
	uint32_t ejtag_ctrl;
	int64_t start = timeval_ms();

	do {
		if (timeval_ms() - start > 1000) {
			LOG_ERROR("DMA time out");
			return -ETIMEDOUT;
		}
		ejtag_ctrl = EJTAG_CTRL_DMAACC | ejtag_info->ejtag_ctrl;
		mips_ejtag_drscan_32(ejtag_info, &ejtag_ctrl);
	} while (ejtag_ctrl & EJTAG_CTRL_DSTRT);
	return 0;
}

static void convert_32_to_16(const uint32_t addr, const uint32_t data_in,
		uint16_t *data_out)
{
	if (addr & 0x2)
		*data_out = (data_in >> 16) & 0xffff;
	else
		*data_out = (data_in & 0x0000ffff);
}

static void convert_32_to_8(const uint32_t addr, const uint32_t data_in,
		uint8_t *data_out)
{
	switch (addr & 0x3) {
		case 0:
			*data_out = data_in & 0xff;
			break;
		case 1:
			*data_out = (data_in >> 8) & 0xff;
			break;
		case 2:
			*data_out = (data_in >> 16) & 0xff;
			break;
		case 3:
			*data_out = (data_in >> 24) & 0xff;
			break;
	}
}

static int ejtag_ctrl_set_dma_size(uint32_t *ejtag_ctrl,
		unsigned int size)
{
	switch (size) {
		case 4:
			*ejtag_ctrl |= EJTAG_CTRL_DMA_WORD;
			break;
		case 2:
			*ejtag_ctrl |= EJTAG_CTRL_DMA_HALFWORD;
			break;
		case 1:
			*ejtag_ctrl |= EJTAG_CTRL_DMA_BYTE;
			break;
		default:
			LOG_ERROR("wrong size %i", size);
			return ERROR_FAIL;
	}
	return ERROR_OK;
}

static int ejtag_dma_read_main(struct mips_ejtag *ejtag_info, uint32_t addr,
		void *data, unsigned int size)
{
	uint32_t v;
	uint32_t ejtag_ctrl = 0, data_out;
	int retries = RETRY_ATTEMPTS;
	int ret;

begin_ejtag_dma_read:

	/* Setup Address */
	v = addr;
	mips_ejtag_set_instr(ejtag_info, EJTAG_INST_ADDRESS);
	mips_ejtag_drscan_32(ejtag_info, &v);

	/* Initiate DMA Read & set DSTRT */
	mips_ejtag_set_instr(ejtag_info, EJTAG_INST_CONTROL);

	ret = ejtag_ctrl_set_dma_size(&ejtag_ctrl, size);
	if (ret)
		goto exit_error;

	ejtag_ctrl |= EJTAG_CTRL_DMAACC | EJTAG_CTRL_DRWN | EJTAG_CTRL_DSTRT
		    | ejtag_info->ejtag_ctrl;
	mips_ejtag_drscan_32(ejtag_info, &ejtag_ctrl);

	/* Wait for DSTRT to Clear */
	ret = ejtag_dma_dstrt_poll(ejtag_info);
	if (ret)
		goto exit_error;

	/* Read Data */
	mips_ejtag_set_instr(ejtag_info, EJTAG_INST_DATA);
	mips_ejtag_drscan_32(ejtag_info, &data_out);

	/* Clear DMA & Check DERR */
	mips_ejtag_set_instr(ejtag_info, EJTAG_INST_CONTROL);
	ejtag_ctrl = ejtag_info->ejtag_ctrl;
	mips_ejtag_drscan_32(ejtag_info, &ejtag_ctrl);

	if (ejtag_ctrl  & EJTAG_CTRL_DERR) {
		if (retries--) {
			LOG_ERROR("DMA Read Addr = %08" PRIx32 "  Data = ERROR ON READ (retrying)", addr);
			goto begin_ejtag_dma_read;
		} else {
			ret = ERROR_JTAG_DEVICE_ERROR;
			goto exit_error;
		}
	}

	switch (size) {
		case 4:
			memcpy(data, &data_out, 4);
			break;
		case 2:
			convert_32_to_16(addr, data_out, (uint16_t *)data);
			break;
		case 1:
			convert_32_to_8(addr, data_out, (uint8_t *)data);
			break;
		default:
			break;
	}
	return ERROR_OK;
exit_error:
	LOG_ERROR("DMA Read Addr = %08" PRIx32 "  Data = ERROR ON READ", addr);
	return ret;
}


static int ejtag_dma_write_main(struct mips_ejtag *ejtag_info, uint32_t addr,
		const void *data, unsigned int size)
{
	uint32_t v, tmp_data;
	uint32_t ejtag_ctrl;
	int retries = RETRY_ATTEMPTS;
	int ret;

begin_ejtag_dma_write:
	/* set dma size first, since it will do sanity check */
	ejtag_ctrl = 0;
	ret = ejtag_ctrl_set_dma_size(&ejtag_ctrl, size);
	if (ret)
		goto exit_error;

	memcpy(&tmp_data, data, size);
	if (size == 2) {
		tmp_data &= 0xffff;
		tmp_data |= tmp_data << 16;
	} else if (size == 1) {
		tmp_data &= 0xff;
		tmp_data |= tmp_data << 8;
		tmp_data |= tmp_data << 16;
	}

	/* Setup Address */
	v = addr;
	mips_ejtag_set_instr(ejtag_info, EJTAG_INST_ADDRESS);
	mips_ejtag_drscan_32(ejtag_info, &v);

	/* Setup Data */
	mips_ejtag_set_instr(ejtag_info, EJTAG_INST_DATA);
	mips_ejtag_drscan_32(ejtag_info, &tmp_data);

	/* Initiate DMA Write & set DSTRT */
	mips_ejtag_set_instr(ejtag_info, EJTAG_INST_CONTROL);


	ejtag_ctrl |= EJTAG_CTRL_DMAACC | EJTAG_CTRL_DSTRT
		    | ejtag_info->ejtag_ctrl;
	mips_ejtag_drscan_32(ejtag_info, &ejtag_ctrl);

	/* Wait for DSTRT to Clear */
	ret = ejtag_dma_dstrt_poll(ejtag_info);
	if (ret)
		goto exit_error;

	/* Clear DMA & Check DERR */
	mips_ejtag_set_instr(ejtag_info, EJTAG_INST_CONTROL);
	ejtag_ctrl = ejtag_info->ejtag_ctrl;
	mips_ejtag_drscan_32(ejtag_info, &ejtag_ctrl);
	if (ejtag_ctrl  & EJTAG_CTRL_DERR) {
		if (retries--) {
			LOG_ERROR("DMA Write Addr = %08" PRIx32 "  Data = ERROR ON WRITE (retrying)", addr);
			goto begin_ejtag_dma_write;
		} else {
			ret = ERROR_JTAG_DEVICE_ERROR;
			goto exit_error;
		}
	}

	return ERROR_OK;
exit_error:
	LOG_ERROR("DMA Write Addr = %08" PRIx32 "  Data = ERROR ON WRITE", addr);
	return ret;
}

int mips32_dmaacc_read_mem(struct mips_ejtag *ejtag_info, uint32_t addr,
		int size, int count, void *buf)
{
	int i;
	int ret;
	uint32_t tmp_addr = addr;
	void *tmp_buf = buf;

	for (i = 0; i < count; i++) {
		ret = ejtag_dma_read_main(ejtag_info, tmp_addr, tmp_buf, size);
		if (ret != ERROR_OK)
			return ret;
		tmp_addr += size;
		tmp_buf += size;
	}

	return ERROR_OK;
}

int mips32_dmaacc_write_mem(struct mips_ejtag *ejtag_info, uint32_t addr,
		int size, int count, const void *buf)
{
	int i;
	int ret;
	uint32_t tmp_addr = addr;
	const void *tmp_buf = buf;

	for (i = 0; i < count; i++) {
		ret = ejtag_dma_write_main(ejtag_info, tmp_addr, tmp_buf, size);
		if (ret != ERROR_OK)
			return ret;
		tmp_addr += size;
		tmp_buf += size;
	}

	return ERROR_OK;

}
