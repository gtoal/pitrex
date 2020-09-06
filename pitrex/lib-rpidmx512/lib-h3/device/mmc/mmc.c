/**
 * @file mmc.c
 *
 */
/* Copyright (C) 2018 by Arjan van Vught mailto:info@raspberrypi-dmx.nl
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
/**
 * Original code : https://github.com/allwinner-zh/bootloader/blob/master/basic_loader/boot0/load_boot1_from_sdmmc/bsp_mmc_for_boot/mmc.c
 */
/*
 * (C) Copyright 2007-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Jerry Wang <wangflord@allwinnertech.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <stddef.h>
#include <string.h>

#include "mmc_internal.h"

#include "h3_timer.h"

#include "debug.h"

#define __be32_to_cpu(x)	((0x000000ff&((x)>>24)) | (0x0000ff00&((x)>>8)) | 			\
							 (0x00ff0000&((x)<< 8)) | (0xff000000&((x)<<24)))

/* Set block count limit because of 16 bit register limit on some hardware*/
#ifndef CONFIG_SYS_MMC_MAX_BLK_COUNT
 #define CONFIG_SYS_MMC_MAX_BLK_COUNT 65535
#endif

static struct mmc* mmc_devices[MAX_MMC_NUM];

int mmc_send_cmd(struct mmc *mmc, struct mmc_cmd *cmd, struct mmc_data *data) {
	return mmc->send_cmd(mmc, cmd, data);
}

int mmc_send_status(struct mmc *mmc, int timeout) {
	struct mmc_cmd cmd;
	int err;

	cmd.cmdidx = MMC_CMD_SEND_STATUS;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = mmc->rca << 16;
	cmd.flags = 0;

	do {
		err = mmc_send_cmd(mmc, &cmd, NULL);
		if (err){
			mmcinfo("mmc %d Send status failed\n",mmc->control_num);
			return err;
		}
		else if (cmd.response[0] & MMC_STATUS_RDY_FOR_DATA)
			break;

		__msdelay(1);

		if (cmd.response[0] & MMC_STATUS_MASK) {
			mmcinfo("mmc %d Status Error: 0x%08X\n",mmc->control_num, cmd.response[0]);
			return COMM_ERR;
		}
	} while (timeout--);

	if (!timeout) {
		mmcinfo("mmc %d Timeout waiting card ready\n",mmc->control_num);
		return TIMEOUT;
	}

	return 0;
}

int mmc_set_blocklen(struct mmc *mmc, int len) {
	struct mmc_cmd cmd;

	cmd.cmdidx = MMC_CMD_SET_BLOCKLEN;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = len;
	cmd.flags = 0;

	return mmc_send_cmd(mmc, &cmd, NULL);
}

struct mmc *find_mmc_device(int dev_num) {
	if (mmc_devices[dev_num] != NULL) {
		return mmc_devices[dev_num];
	}

	mmcinfo("MMC Device %d not found\n", dev_num);

	return NULL;
}

#ifdef SD_WRITE_SUPPORT
unsigned mmc_write_blocks(struct mmc *mmc, unsigned long start, unsigned blkcnt, const void*src) {
	DEBUG1_ENTRY

	struct mmc_cmd cmd;
	struct mmc_data data;
	int timeout = 1000;

	if ((start + blkcnt) > mmc->lba) {
		mmcinfo("block number 0x%lx exceeds max(0x%lx)\n", start + blkcnt, mmc->lba);
		DEBUG1_EXIT
		return 0;
	}

	if (blkcnt > 1)
		cmd.cmdidx = MMC_CMD_WRITE_MULTIPLE_BLOCK;
	else
		cmd.cmdidx = MMC_CMD_WRITE_SINGLE_BLOCK;

	if (mmc->high_capacity)
		cmd.cmdarg = start;
	else
		cmd.cmdarg = start * mmc->write_bl_len;

	cmd.resp_type = MMC_RSP_R1;
	cmd.flags = 0;

	data.b.src = src;
	data.blocks = blkcnt;
	data.blocksize = mmc->write_bl_len;
	data.flags = MMC_DATA_WRITE;

	if (mmc_send_cmd(mmc, &cmd, &data)) {
		mmcinfo("mmc %d mmc write failed\n",mmc->control_num);
		DEBUG1_EXIT
		return 0;
	}

	/* SPI multiblock writes terminate using a special
	 * token, not a STOP_TRANSMISSION request.
	 */
	if (!mmc_host_is_spi(mmc) && blkcnt > 1) {
		cmd.cmdidx = MMC_CMD_STOP_TRANSMISSION;
		cmd.cmdarg = 0;
		cmd.resp_type = MMC_RSP_R1b;
		cmd.flags = 0;
		if (mmc_send_cmd(mmc, &cmd, NULL)) {
			mmcinfo("mmc %d fail to send stop cmd\n",mmc->control_num);
			DEBUG1_EXIT
			return 0;
		}


	}

	/* Waiting for the ready status */
	mmc_send_status(mmc, timeout);

	DEBUG1_EXIT
	return blkcnt;
}
#endif

int mmc_read_blocks(struct mmc *mmc, void *dst, unsigned long start, unsigned blkcnt) {
	DEBUG1_ENTRY

	struct mmc_cmd cmd;
	struct mmc_data data;
	int timeout = 1000;

	if (blkcnt > 1)
		cmd.cmdidx = MMC_CMD_READ_MULTIPLE_BLOCK;
	else
		cmd.cmdidx = MMC_CMD_READ_SINGLE_BLOCK;

	if (mmc->high_capacity)
		cmd.cmdarg = start;
	else
		cmd.cmdarg = start * mmc->read_bl_len;

	cmd.resp_type = MMC_RSP_R1;
	cmd.flags = 0;

	data.b.dest = dst;
	data.blocks = blkcnt;
	data.blocksize = mmc->read_bl_len;
	data.flags = MMC_DATA_READ;

	if (mmc_send_cmd(mmc, &cmd, &data)){
		mmcinfo("read block failed\n");
		DEBUG1_EXIT
		return 0;
	}

	if (blkcnt > 1) {
		cmd.cmdidx = MMC_CMD_STOP_TRANSMISSION;
		cmd.cmdarg = 0;
		cmd.resp_type = MMC_RSP_R1b;
		cmd.flags = 0;
		if (mmc_send_cmd(mmc, &cmd, NULL)) {
			mmcinfo("fail to send stop cmd\n");
			DEBUG1_EXIT
			return 0;
		}

		/* Waiting for the ready status */
		mmc_send_status(mmc, timeout);
	}

	DEBUG1_EXIT
	return blkcnt;
}

int mmc_go_idle(struct mmc* mmc) {
	struct mmc_cmd cmd;
	int err;

	__msdelay(1);

	cmd.cmdidx = MMC_CMD_GO_IDLE_STATE;
	cmd.cmdarg = 0;
	cmd.resp_type = MMC_RSP_NONE;
	cmd.flags = 0;

	err = mmc_send_cmd(mmc, &cmd, NULL);

	if (err) {
		mmcinfo("mmc %d go idle failed\n", mmc->control_num);
		return err;
	}

	__msdelay(2);

	return 0;
}

int sd_send_op_cond(struct mmc *mmc) {
	int timeout = 1000;
	int err;
	struct mmc_cmd cmd;

	do {
		cmd.cmdidx = MMC_CMD_APP_CMD;
		cmd.resp_type = MMC_RSP_R1;
		cmd.cmdarg = 0;
		cmd.flags = 0;

		err = mmc_send_cmd(mmc, &cmd, NULL);

		if (err){
			mmcinfo("mmc %d send app cmd failed\n",mmc->control_num);
			return err;
		}

		cmd.cmdidx = SD_CMD_APP_SEND_OP_COND;
		cmd.resp_type = MMC_RSP_R3;

		/*
		 * Most cards do not answer if some reserved bits
		 * in the ocr are set. However, Some controller
		 * can set bit 7 (reserved for low voltages), but
		 * how to manage low voltages SD card is not yet
		 * specified.
		 */
		cmd.cmdarg = mmc_host_is_spi(mmc) ? 0 :
			(mmc->voltages & 0xff8000);

		if (mmc->version == SD_VERSION_2)
			cmd.cmdarg |= OCR_HCS;

		err = mmc_send_cmd(mmc, &cmd, NULL);

		if (err){
			mmcinfo("mmc %d send cmd41 failed\n",mmc->control_num);
			return err;
		}

		__msdelay(1);
	} while ((!(cmd.response[0] & OCR_BUSY)) && timeout--);

	if (timeout <= 0){
		mmcinfo("mmc %d wait card init failed\n",mmc->control_num);
		return UNUSABLE_ERR;
	}

	if (mmc->version != SD_VERSION_2)
		mmc->version = SD_VERSION_1_0;

	if (mmc_host_is_spi(mmc)) { /* read OCR for spi */
		cmd.cmdidx = MMC_CMD_SPI_READ_OCR;
		cmd.resp_type = MMC_RSP_R3;
		cmd.cmdarg = 0;
		cmd.flags = 0;

		err = mmc_send_cmd(mmc, &cmd, NULL);

		if (err){
			mmcinfo("mmc %d spi read ocr failed\n",mmc->control_num);
			return err;
		}
	}

	mmc->ocr = cmd.response[0];

	mmc->high_capacity = ((mmc->ocr & OCR_HCS) == OCR_HCS);
	mmc->rca = 0;

	return 0;
}

int mmc_send_op_cond(struct mmc *mmc)
{
	int timeout = 10000;
	struct mmc_cmd cmd;
	int err;

	/* Some cards seem to need this */
	mmc_go_idle(mmc);

 	/* Asking to the card its capabilities */
 	cmd.cmdidx = MMC_CMD_SEND_OP_COND;
 	cmd.resp_type = MMC_RSP_R3;
 	cmd.cmdarg = 0x40ff8000;//foresee
 	cmd.flags = 0;

  //mmcinfo("mmc send op cond arg not zero !!!\n");
 	err = mmc_send_cmd(mmc, &cmd, NULL);

 	if (err){
 		mmcinfo("mmc %d send op cond failed\n",mmc->control_num);
 		return err;
 	}

 	__msdelay(1);

	do {
		cmd.cmdidx = MMC_CMD_SEND_OP_COND;
		cmd.resp_type = MMC_RSP_R3;
		cmd.cmdarg = (mmc_host_is_spi(mmc) ? 0 :
				(mmc->voltages &
				(cmd.response[0] & OCR_VOLTAGE_MASK)) |
				(cmd.response[0] & OCR_ACCESS_MODE));

		if (mmc->host_caps & MMC_MODE_HC)
			cmd.cmdarg |= OCR_HCS;

		cmd.flags = 0;

		err = mmc_send_cmd(mmc, &cmd, NULL);

		if (err){
			mmcinfo("mmc %d send op cond failed\n",mmc->control_num);
			return err;
		}

		__msdelay(1);
	} while (!(cmd.response[0] & OCR_BUSY) && timeout--);

	if (timeout <= 0){
		mmcinfo("mmc %d wait for mmc init failed\n",mmc->control_num);
		return UNUSABLE_ERR;
	}

	if (mmc_host_is_spi(mmc)) { /* read OCR for spi */
		cmd.cmdidx = MMC_CMD_SPI_READ_OCR;
		cmd.resp_type = MMC_RSP_R3;
		cmd.cmdarg = 0;
		cmd.flags = 0;

		err = mmc_send_cmd(mmc, &cmd, NULL);
		if (err)
			return err;
	}

	mmc->version = MMC_VERSION_UNKNOWN;
	mmc->ocr = cmd.response[0];

	mmc->high_capacity = ((mmc->ocr & OCR_HCS) == OCR_HCS);
	mmc->rca = 1;

	return 0;
}

int mmc_send_ext_csd(struct mmc *mmc, char *ext_csd) {
	struct mmc_cmd cmd;
	struct mmc_data data;
	int err;

	/* Get the Card Status Register */
	cmd.cmdidx = MMC_CMD_SEND_EXT_CSD;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = 0;
	cmd.flags = 0;

	data.b.dest = ext_csd;
	data.blocks = 1;
	data.blocksize = 512;
	data.flags = MMC_DATA_READ;

	err = mmc_send_cmd(mmc, &cmd, &data);

	if (err) {
		mmcinfo("mmc %d send ext csd failed\n", mmc->control_num);
	}

	return err;
}

int mmc_switch(struct mmc *mmc, uint8_t set, uint8_t index, uint8_t value) {
	struct mmc_cmd cmd;
	int timeout = 1000;
	int ret;

	cmd.cmdidx = MMC_CMD_SWITCH;
	cmd.resp_type = MMC_RSP_R1b;
	cmd.cmdarg = (MMC_SWITCH_MODE_WRITE_BYTE << 24) | (index << 16) | (value << 8);
	cmd.flags = 0;

	ret = mmc_send_cmd(mmc, &cmd, NULL);
	if (ret) {
		mmcinfo("mmc %d switch failed\n", mmc->control_num);
	}

	/* Waiting for the ready status */
	mmc_send_status(mmc, timeout);

	return ret;

}

int mmc_change_freq(struct mmc *mmc) {
	char ext_csd[512];
	char cardtype;
	int err;
	int retry = 5;

	mmc->card_caps = 0;

	if (mmc_host_is_spi(mmc)) {
		return 0;
	}

	/* Only version 4 supports high-speed */
	if (mmc->version < MMC_VERSION_4) {
		return 0;
	}

	mmc->card_caps |= MMC_MODE_4BIT;

	err = mmc_send_ext_csd(mmc, ext_csd);

	if (err){
		mmcinfo("mmc %d get ext csd failed\n",mmc->control_num);
		return err;
	}

	cardtype = ext_csd[196] & 0xf;

		//retry for Toshiba emmc,for the first time Toshiba emmc change to HS
	//it will return response crc err,so retry
	do{
		err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_HS_TIMING, 1);
		if(!err){
			break;
		}
		mmcinfo("retry mmc switch(cmd6)\n");
	}while(retry--);

	if (err){
		mmcinfo("mmc %d change to hs failed\n",mmc->control_num);
		return err;
	}

	/* Now check to see that it worked */
	err = mmc_send_ext_csd(mmc, ext_csd);

	if (err){
		mmcinfo("mmc %d send ext csd faild\n",mmc->control_num);
		return err;
	}

	/* No high-speed support */
	if (!ext_csd[185])
		return 0;

	/* High Speed is set, there are two types: 52MHz and 26MHz */
	if (cardtype & MMC_HS_52MHZ)
		mmc->card_caps |= MMC_MODE_HS_52MHz | MMC_MODE_HS;
	else
		mmc->card_caps |= MMC_MODE_HS;

	return 0;
}

int mmc_switch_part(int dev_num, unsigned int part_num) {
	struct mmc *mmc = find_mmc_device(dev_num);

	if (!mmc) {
		return -1;
	}

	return mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_PART_CONF,
			(mmc->part_config & ~PART_ACCESS_MASK) | (part_num & PART_ACCESS_MASK));
}

int sd_switch(struct mmc *mmc, int mode, int group, uint8_t value, uint8_t *resp)
{
	struct mmc_cmd cmd;
	struct mmc_data data;

	/* Switch the frequency */
	cmd.cmdidx = SD_CMD_SWITCH_FUNC;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = (mode << 31) | 0xffffff;
	cmd.cmdarg &= ~(0xf << (group * 4));
	cmd.cmdarg |= value << (group * 4);
	cmd.flags = 0;

	data.b.dest = (char *)resp;
	data.blocksize = 64;
	data.blocks = 1;
	data.flags = MMC_DATA_READ;

	return mmc_send_cmd(mmc, &cmd, &data);
}

int sd_change_freq(struct mmc *mmc) {
	int err;
	struct mmc_cmd cmd;
	uint32_t scr[2];
	uint32_t switch_status[16];
	struct mmc_data data;
	int timeout;

	mmc->card_caps = 0;

	if (mmc_host_is_spi(mmc)) {
		return 0;
	}

	/* Read the SCR to find out if this card supports higher speeds */
	cmd.cmdidx = MMC_CMD_APP_CMD;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = mmc->rca << 16;
	cmd.flags = 0;

	err = mmc_send_cmd(mmc, &cmd, NULL);

	if (err){
		mmcinfo("mmc %d Send app cmd failed\n",mmc->control_num);
		return err;
	}

	cmd.cmdidx = SD_CMD_APP_SEND_SCR;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = 0;
	cmd.flags = 0;

	timeout = 3;

retry_scr:
	data.b.dest = (char *)&scr;
	data.blocksize = 8;
	data.blocks = 1;
	data.flags = MMC_DATA_READ;

	err = mmc_send_cmd(mmc, &cmd, &data);

	if (err) {
		if (timeout--)
			goto retry_scr;

		mmcinfo("mmc %d Send scr failed\n",mmc->control_num);
		return err;
	}

	mmc->scr[0] = __be32_to_cpu(scr[0]);
	mmc->scr[1] = __be32_to_cpu(scr[1]);

	switch ((mmc->scr[0] >> 24) & 0xf) {
		case 0:
			mmc->version = SD_VERSION_1_0;
			break;
		case 1:
			mmc->version = SD_VERSION_1_10;
			break;
		case 2:
			mmc->version = SD_VERSION_2;
			break;
		default:
			mmc->version = SD_VERSION_1_0;
			break;
	}

	if (mmc->scr[0] & SD_DATA_4BIT)
		mmc->card_caps |= MMC_MODE_4BIT;

	/* Version 1.0 doesn't support switching */
	if (mmc->version == SD_VERSION_1_0)
		return 0;

	timeout = 4;
	while (timeout--) {
		err = sd_switch(mmc, SD_SWITCH_CHECK, 0, 1,
				(uint8_t *)&switch_status);

		if (err){
			mmcinfo("mmc %d Check high speed status faild\n",mmc->control_num);
			return err;
		}

		/* The high-speed function is busy.  Try again */
		if (!(__be32_to_cpu(switch_status[7]) & SD_HIGHSPEED_BUSY))
			break;
	}

	/* If high-speed isn't supported, we return */
	if (!(__be32_to_cpu(switch_status[3]) & SD_HIGHSPEED_SUPPORTED))
		return 0;

	err = sd_switch(mmc, SD_SWITCH_SWITCH, 0, 1, (uint8_t *)&switch_status);

	if (err){
		mmcinfo("mmc %d switch to high speed failed\n",mmc->control_num);
		return err;
	}

	if ((__be32_to_cpu(switch_status[4]) & 0x0f000000) == 0x01000000)
		mmc->card_caps |= MMC_MODE_HS;

	return 0;
}

/* frequency bases */
/* divided by 10 to be nice to platforms without floating point */
static const int fbase[] = {
	10000,
	100000,
	1000000,
	10000000,
};

/* Multiplier values for TRAN_SPEED.  Multiplied by 10 to be nice
 * to platforms without floating point.
 */
static const int multipliers[] = { 0, /* reserved */
10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80, };

void mmc_set_ios(struct mmc *mmc) {
	mmc->set_ios(mmc);
}

void mmc_set_clock(struct mmc *mmc, uint32_t clock) {
	if (clock > mmc->f_max)
		clock = mmc->f_max;

	if (clock < mmc->f_min)
		clock = mmc->f_min;

	mmc->clock = clock;

	mmc_set_ios(mmc);
}

void mmc_set_bus_width(struct mmc *mmc, uint32_t width) {
	mmc->bus_width = width;

	mmc_set_ios(mmc);
}

int mmc_startup(struct mmc *mmc) {
	DEBUG1_ENTRY

	int err;
	uint32_t mult, freq;
	uint64_t cmult, csize, capacity;
	struct mmc_cmd cmd;
	char ext_csd[512];
	int timeout = 1000;

	/* Put the Card in Identify Mode */
	cmd.cmdidx = mmc_host_is_spi(mmc) ? MMC_CMD_SEND_CID :
		MMC_CMD_ALL_SEND_CID; /* cmd not supported in spi */
	cmd.resp_type = MMC_RSP_R2;
	cmd.cmdarg = 0;
	cmd.flags = 0;

	err = mmc_send_cmd(mmc, &cmd, NULL);

	if (err){
		mmcinfo("mmc %d Put the Card in Identify Mode failed\n",mmc->control_num);
		DEBUG1_EXIT
		return err;
	}

	memcpy(mmc->cid, cmd.response, 16);

	/*
	 * For MMC cards, set the Relative Address.
	 * For SD cards, get the Relatvie Address.
	 * This also puts the cards into Standby State
	 */
	if (!mmc_host_is_spi(mmc)) { /* cmd not supported in spi */
		cmd.cmdidx = SD_CMD_SEND_RELATIVE_ADDR;
		cmd.cmdarg = mmc->rca << 16;
		cmd.resp_type = MMC_RSP_R6;
		cmd.flags = 0;

		err = mmc_send_cmd(mmc, &cmd, NULL);

		if (err){
			mmcinfo("mmc %d send rca failed\n",mmc->control_num);
			DEBUG1_EXIT
			return err;
		}

		if (IS_SD(mmc))
			mmc->rca = (cmd.response[0] >> 16) & 0xffff;
	}

	/* Get the Card-Specific Data */
	cmd.cmdidx = MMC_CMD_SEND_CSD;
	cmd.resp_type = MMC_RSP_R2;
	cmd.cmdarg = mmc->rca << 16;
	cmd.flags = 0;

	err = mmc_send_cmd(mmc, &cmd, NULL);

	/* Waiting for the ready status */
	mmc_send_status(mmc, timeout);

	if (err){
		mmcinfo("mmc %d get csd failed\n",mmc->control_num);
		DEBUG1_EXIT
		return err;
	}

	mmc->csd[0] = cmd.response[0];
	mmc->csd[1] = cmd.response[1];
	mmc->csd[2] = cmd.response[2];
	mmc->csd[3] = cmd.response[3];

	if (mmc->version == MMC_VERSION_UNKNOWN) {
		int version = (cmd.response[0] >> 26) & 0xf;

		switch (version) {
			case 0:
				mmc->version = MMC_VERSION_1_2;
				break;
			case 1:
				mmc->version = MMC_VERSION_1_4;
				break;
			case 2:
				mmc->version = MMC_VERSION_2_2;
				break;
			case 3:
				mmc->version = MMC_VERSION_3;
				break;
			case 4:
				mmc->version = MMC_VERSION_4;
				break;
			default:
				mmc->version = MMC_VERSION_1_2;
				break;
		}
	}

	/* divide frequency by 10, since the mults are 10x bigger */
	freq = fbase[(cmd.response[0] & 0x7)];
	mult = multipliers[((cmd.response[0] >> 3) & 0xf)];

	mmc->tran_speed = freq * mult;

	mmc->read_bl_len = 1 << ((cmd.response[1] >> 16) & 0xf);

	if (IS_SD(mmc))
		mmc->write_bl_len = mmc->read_bl_len;
	else
		mmc->write_bl_len = 1 << ((cmd.response[3] >> 22) & 0xf);

	if (mmc->high_capacity) {
		csize = (mmc->csd[1] & 0x3f) << 16
			| (mmc->csd[2] & 0xffff0000) >> 16;
		cmult = 8;
	} else {
		csize = (mmc->csd[1] & 0x3ff) << 2
			| (mmc->csd[2] & 0xc0000000) >> 30;
		cmult = (mmc->csd[2] & 0x00038000) >> 15;
	}

	mmc->capacity = (csize + 1) << (cmult + 2);
	mmc->capacity *= mmc->read_bl_len;

	if (mmc->read_bl_len > 512)
		mmc->read_bl_len = 512;

	if (mmc->write_bl_len > 512)
		mmc->write_bl_len = 512;

	if (IS_SD(mmc)) {
		mmc_set_clock(mmc, 25000000);
	}

	/* Select the card, and put it into Transfer Mode */
	if (!mmc_host_is_spi(mmc)) { /* cmd not supported in spi */
		cmd.cmdidx = MMC_CMD_SELECT_CARD;
		cmd.resp_type = MMC_RSP_R1b;
		cmd.cmdarg = mmc->rca << 16;
		cmd.flags = 0;
		err = mmc_send_cmd(mmc, &cmd, NULL);

		if (err){
			mmcinfo("Select the card failed\n");
			DEBUG1_EXIT
			return err;
		}
	}

	/*
	 * For SD, its erase group is always one sector
	 */
	mmc->erase_grp_size = 1;
	mmc->part_config = MMCPART_NOAVAILABLE;
	if (!IS_SD(mmc) && (mmc->version >= MMC_VERSION_4)) {
		/* check  ext_csd version and capacity */
		err = mmc_send_ext_csd(mmc, ext_csd);

		if(!err){
				/* update mmc version */
			switch (ext_csd[192]) {
				case 0:
					mmc->version = MMC_VERSION_4;
					break;
				case 1:
					mmc->version = MMC_VERSION_4_1;
					break;
				case 2:
					mmc->version = MMC_VERSION_4_2;
					break;
				case 3:
					mmc->version = MMC_VERSION_4_3;
					break;
				case 5:
					mmc->version = MMC_VERSION_4_41;
					break;
				case 6:
					mmc->version = MMC_VERSION_4_5;
					break;
				case 7:
					mmc->version = MMC_VERSION_5_0;
					break;
			}
		}


		if (!err & (ext_csd[192] >= 2)) {
			/*
			 * According to the JEDEC Standard, the value of
			 * ext_csd's capacity is valid if the value is more
			 * than 2GB
			 */
			capacity = ext_csd[212] << 0 | ext_csd[213] << 8 |
				   ext_csd[214] << 16 | ext_csd[215] << 24;
			capacity *= 512;
			if ((capacity >> 20) > 2 * 1024)
				mmc->capacity = capacity;
		}

		/*
		 * Check whether GROUP_DEF is set, if yes, read out
		 * group size from ext_csd directly, or calculate
		 * the group size from the csd value.
		 */
		if (ext_csd[175])
			mmc->erase_grp_size = ext_csd[224] * 512 * 1024;
		else {
			int erase_gsz, erase_gmul;
			erase_gsz = (mmc->csd[2] & 0x00007c00) >> 10;
			erase_gmul = (mmc->csd[2] & 0x000003e0) >> 5;
			mmc->erase_grp_size = (erase_gsz + 1)
				* (erase_gmul + 1);
		}

		/* store the partition info of emmc */
		if (ext_csd[160] & PART_SUPPORT)
			mmc->part_config = ext_csd[179];
	}

	mmc_set_clock(mmc,25000000);

	if (IS_SD(mmc))
		err = sd_change_freq(mmc);
	else
		err = mmc_change_freq(mmc);

	if (err){
		mmcinfo("mmc %d Change speed mode failed\n",mmc->control_num);
		DEBUG1_EXIT
		return err;
	}

	/* Restrict card's capabilities by what the host can do */
	mmc->card_caps &= mmc->host_caps;

	if (IS_SD(mmc)) {
		if (mmc->card_caps & MMC_MODE_4BIT) {
			cmd.cmdidx = MMC_CMD_APP_CMD;
			cmd.resp_type = MMC_RSP_R1;
			cmd.cmdarg = mmc->rca << 16;
			cmd.flags = 0;

			err = mmc_send_cmd(mmc, &cmd, NULL);
			if (err){
				mmcinfo("mmc %d send app cmd failed\n",mmc->control_num);
				return err;
			}

			cmd.cmdidx = SD_CMD_APP_SET_BUS_WIDTH;
			cmd.resp_type = MMC_RSP_R1;
			cmd.cmdarg = 2;
			cmd.flags = 0;
			err = mmc_send_cmd(mmc, &cmd, NULL);
			if (err){
				mmcinfo("mmc %d sd set bus width failed\n",mmc->control_num);
				return err;
			}

			mmc_set_bus_width(mmc, 4);
		}

		if (mmc->card_caps & MMC_MODE_HS)
			mmc_set_clock(mmc, 50000000);
		else
			mmc_set_clock(mmc, 25000000);
	} else {
		if (mmc->card_caps & MMC_MODE_4BIT) {
			/* Set the card to use 4 bit*/
			err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL,
					EXT_CSD_BUS_WIDTH,
					EXT_CSD_BUS_WIDTH_4);

			if (err){
				mmcinfo("mmc %d switch bus width failed\n",mmc->control_num);
				return err;
			}

			mmc_set_bus_width(mmc, 4);
		} else if (mmc->card_caps & MMC_MODE_8BIT) {
			/* Set the card to use 8 bit*/
			err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL,
					EXT_CSD_BUS_WIDTH,
					EXT_CSD_BUS_WIDTH_8);

			if (err){
				mmcinfo("mmc %d switch bus width8 failed\n",mmc->control_num);
				return err;
			}

			mmc_set_bus_width(mmc, 8);
		}

		if (mmc->card_caps & MMC_MODE_HS) {
			if (mmc->card_caps & MMC_MODE_HS_52MHz)
				mmc_set_clock(mmc, 52000000);
			else
				mmc_set_clock(mmc, 26000000);
		} else
			mmc_set_clock(mmc, 20000000);
	}

	/* fill in device description */
	mmc->blksz = mmc->read_bl_len;
	mmc->lba = mmc->capacity/mmc->read_bl_len;

	if(!IS_SD(mmc)){
		switch(mmc->version)
		{
			case MMC_VERSION_1_2:
				mmcinfo("MMC ver 1.2\n");
				break;
			case MMC_VERSION_1_4:
				mmcinfo("MMC ver 1.4\n");
				break;
			case MMC_VERSION_2_2:
				mmcinfo("MMC ver 2.2\n");
				break;
			case MMC_VERSION_3:
				mmcinfo("MMC ver 3.0\n");
				break;
			case MMC_VERSION_4:
				mmcinfo("MMC ver 4.0\n");
				break;
			case MMC_VERSION_4_1:
				mmcinfo("MMC ver 4.1\n");
				break;
			case MMC_VERSION_4_2:
				mmcinfo("MMC ver 4.2\n");
				break;
			case MMC_VERSION_4_3:
				mmcinfo("MMC ver 4.3\n");
				break;
			case MMC_VERSION_4_41:
				mmcinfo("MMC ver 4.41\n");
				break;
			case MMC_VERSION_4_5:
				mmcinfo("MMC ver 4.5\n");
				break;
			case MMC_VERSION_5_0:
				mmcinfo("MMC ver 5.0\n");
				break;
			default:
				mmcinfo("Unknow MMC ver\n");
				break;
		}
	}
	mmcinfo("SD/MMC Card: %dbit, capacity: %dMB\n",
					mmc->card_caps & MMC_MODE_4BIT ? 4 : 1, mmc->lba >> 11);
	mmcinfo("vendor: Man %x Snr %x\n", (mmc->cid[0] >> 8) & 0xffffff,
					(mmc->cid[2] << 8) | (mmc->cid[3] >> 24));
	mmcinfo("product: %c%c%c%c%c\n", mmc->cid[0] & 0xff,
			(mmc->cid[1] >> 24), (mmc->cid[1] >> 16) & 0xff,
			(mmc->cid[1] >> 8) & 0xff, mmc->cid[1] & 0xff);
	mmcinfo("revision: %d.%d\n", mmc->cid[2] >> 28, (mmc->cid[2] >> 24) & 0xf);

	DEBUG1_EXIT
	return 0;
}

int mmc_send_if_cond(struct mmc *mmc)
{
	struct mmc_cmd cmd;
	int err;

	cmd.cmdidx = SD_CMD_SEND_IF_COND;
	/* We set the bit if the host supports voltages between 2.7 and 3.6 V */
	cmd.cmdarg = ((mmc->voltages & 0xff8000) != 0) << 8 | 0xaa;
	cmd.resp_type = MMC_RSP_R7;
	cmd.flags = 0;

	err = mmc_send_cmd(mmc, &cmd, NULL);

	if (err){
		mmcinfo("mmc %d send if cond failed\n",mmc->control_num);
		return err;
	}

	if ((cmd.response[0] & 0xff) != 0xaa)
		return UNUSABLE_ERR;
	else
		mmc->version = SD_VERSION_2;

	return 0;
}

int mmc_init(struct mmc *mmc) {
	DEBUG1_ENTRY

	int err;

	if (mmc->has_init) {
		mmcinfo("mmc %d Has init\n",mmc->control_num);
		DEBUG1_EXIT
		return 0;
	}

	err = mmc->init();
	if (err) {
		mmcinfo("mmc %d host init failed\n",mmc->control_num);
		DEBUG1_EXIT
		return err;
	}

	mmc_set_bus_width(mmc, 1);
	mmc_set_clock(mmc, 1);

	/* Reset the Card */
	err = mmc_go_idle(mmc);
	if (err) {
		mmcinfo("mmc %d reset card failed\n",mmc->control_num);
		DEBUG1_EXIT
		return err;
	}

	/* The internal partition reset to user partition(0) at every CMD0*/
	mmc->part_num = 0;

	mmcinfo("***Try SD card %d***\n",mmc->control_num);
	/* Test for SD version 2 */
	err = mmc_send_if_cond(mmc);

	/* Now try to get the SD card's operating condition */
	err = sd_send_op_cond(mmc);

	/* If the command timed out, we check for an MMC card */
	if (err) {
		mmcinfo("***Try MMC card %d***\n",mmc->control_num);
		err = mmc_send_op_cond(mmc);

		if (err) {
			mmcinfo("mmc %d Card did not respond to voltage select!\n",mmc->control_num); mmcinfo("***SD/MMC %d init error!!!***\n",mmc->control_num);
			DEBUG1_EXIT
			return UNUSABLE_ERR;
		}
	}

	err = mmc_startup(mmc);
	if (err) {
		mmcinfo("***SD/MMC %d init error!!!***\n",mmc->control_num);
		mmc->has_init = 0;
	} else {
		mmc->has_init = 1;
		mmcinfo("***SD/MMC %d init OK!!!***\n",mmc->control_num);
	}

	DEBUG1_EXIT
	return err;
}

int mmc_register(int dev_num, struct mmc *mmc) {
	DEBUG1_ENTRY

	mmc_devices[dev_num] = mmc;

	if (!mmc->b_max)
		mmc->b_max = CONFIG_SYS_MMC_MAX_BLK_COUNT;

	DEBUG1_EXIT
	return mmc_init(mmc);
}
