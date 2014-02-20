/*
 * (C) Copyright 2006 DENX Software Engineering
 *
 * Implementation for U-Boot 1.1.6 by Samsung
 *
 * (C) Copyright 2008
 * Guennadi Liakhovetki, DENX Software Engineering, <lg@denx.de>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
 
/*
 * (C) Copyright 2011
 * Dongseok Lee, Samsung Erectronics, <drain.lee@samsung.com>
 *			- Modify for Firenze
 */
#ifdef __KERNEL__
#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/cpufreq.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/partitions.h>

#include <asm/io.h>

#include <mach/sdp1004.h>

#include <plat/sdp_nand.h>
#include <plat/sdp_nand_reg.h>
#else
#include <common.h>

#include <nand.h>
#include <asm/arch/sdp_1004.h>
#include <asm/arch/sdp_nand_reg.h>

#include <asm/io.h>
#include <asm/errno.h>
#endif

#define MAX_CHIPS	4
static int nand_cs[MAX_CHIPS] = {0, 1, 2, 3};

#ifdef CONFIG_NAND_SPL
#define printf(arg...) do {} while (0)
#endif

/* for using #define NFCONF			(SDP_NAND_BASE + NFCONF_OFFSET) */
#ifdef __KERNEL__

#ifdef SDP_NAND_BASE
#undef SDP_NAND_BASE
#endif/*SDP_NAND_BASE*/

static void __iomem *_sdp_nand_base = NULL;
#define SDP_NAND_BASE		_sdp_nand_base
#define printf				printk

#endif/*__KERNEL__*/

	
#ifndef CONFIG_NAND_SPL
static struct nand_ecclayout sdp_oobinfo_64 = {
	.eccbytes = 7,
	.eccpos = {32, 33, 34, 35, 36, 37, 38 ,
				39, 40, 41, 42, 43, 44, 45,
				46, 47, 48, 49, 50, 51, 52,
				53, 54, 55, 56, 57, 58, 59},
	.oobfree = {{1, 24}},
};

static uint8_t sdp_pattern = 0xFF;

static struct nand_bbt_descr sdp_largepage_memorybased = {
	.options = 0,
	.offs = 0,
	.len = 1,
	.pattern = &sdp_pattern,
};
#endif/* !CONFIG_NAND_SPL*/

/*  */
struct sdp_nand_chip_data {
	int nand_cs;
	int cur_realpage;/* block no and page no */
};

static struct sdp_nand_chip_data sdp_nand_data;


#ifdef __KERNEL__

struct sdp_nand_info;

struct sdp_nand_mtd {
	struct mtd_info			mtd;
	struct nand_chip		chip;
	struct sdp_nand_info	*sinfo;
	int scan_result;
};

struct sdp_nand_info {
	/* mtd info structs */
	struct nand_hw_control		controller;
	struct sdp_nand_mtd			*mtds;
	struct sdp_nand_platform	*platform;

	/* device info */
	struct device			*dev;
	struct resource			*res;//for flatform resource
	struct resource			*area;//for request_mem_region
	struct clk				*clk;
	void __iomem			*regbase;//for ioread, iowrite
	int				mtd_count;
};
#endif



/* Nand flash definition values by jsgood */
#ifdef CONFIG_NAND_SDP_DEBUG
/*
 * Function to print out oob buffer for debugging
 * Written by jsgood
 */
static void print_oob(const char *header, struct mtd_info *mtd)
{
	int i;
	struct nand_chip *chip = mtd->priv;

	printf("%s:\n", header);

	for (i = 0; i < 64; i++)
	{
		if(i%16 == 0) printf("\n");
		printf("%02x ", chip->oob_poi[i]);
	}

	printf("\n");
}
#endif /* CONFIG_NAND_SDP_DEBUG */

#ifdef CONFIG_NAND_SPL
static u_char nand_read_byte(struct mtd_info *mtd)
{
	struct nand_chip *this = mtd->priv;
	return readb(this->IO_ADDR_R);
}

static void nand_write_buf(struct mtd_info *mtd, const uint8_t *buf, int len)
{
	int i;
	struct nand_chip *this = mtd->priv;

	for (i = 0; i < len; i++)
		writeb(buf[i], this->IO_ADDR_W);
}

static void nand_read_buf(struct mtd_info *mtd, u_char *buf, int len)
{
	int i;
	struct nand_chip *this = mtd->priv;

	for (i = 0; i < len; i++)
		buf[i] = readb(this->IO_ADDR_R);
}
#endif/*CONFIG_NAND_SPL*/

static void sdp_nand_select_chip(struct mtd_info *mtd, int chip)
{
	int ctrl = readl(NFCONT);
	
	switch (chip) {
	case -1:
		/* All CE is diasble */
		ctrl |= NFCONT_nCE_ALL;
		break;
	case 0:
		ctrl &= ~NFCONT_nCE0;
		break;
	case 1:
		ctrl &= ~~NFCONT_nCE1;
		break;
	case 2:
		ctrl &= ~~NFCONT_nCE2;
		break;
	case 3:
		ctrl &= ~~NFCONT_nCE3;
		break;
	default:
		return;
	}

	writel(ctrl, NFCONT);
}

/*
 * Hardware specific access to control-lines function
 * Written by jsgood
 */
static void sdp_nand_hwcontrol(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{
	struct nand_chip *this = mtd->priv;

	if (ctrl & NAND_CTRL_CHANGE) {
		if (ctrl & NAND_CLE)
			this->IO_ADDR_W = (void __iomem *)NFCMMD;
		else if (ctrl & NAND_ALE)
			this->IO_ADDR_W = (void __iomem *)NFADDR;
		else
			this->IO_ADDR_W = (void __iomem *)NFDATA;
		if (ctrl & NAND_NCE)
			sdp_nand_select_chip(mtd, ((struct sdp_nand_chip_data*)this->priv)->nand_cs);
		else
			sdp_nand_select_chip(mtd, -1);
	}

	if (cmd != NAND_CMD_NONE)
		writeb(cmd, this->IO_ADDR_W);
}

/*
 * Function for checking device ready pin
 * Written by jsgood
 */
static int sdp_nand_device_ready(struct mtd_info *mtdinfo)
{
	return !!(readl(NFSTAT) & NFSTAT_RnB);
}

#ifdef CONFIG_SYS_SDP_NAND_HWECC
/*
 * This function is called before encoding ecc codes to ready ecc engine.
 * Written by jsgood
 */
static void sdp_nand_enable_hwecc_ex(struct mtd_info *mtd, int mode)
{
	NFECC_MAIN_SELECT();

	/* calulate ecc */
	if (mode == NAND_ECC_WRITE) {
		NFECC_ENCODING();
	}
	/* correct ecc */
	else if (mode == NAND_ECC_READ) {
		NFECC_DECODING();
	}

	NFECC_MAIN_RESET();
	NFECC_MAIN_INIT();
	NFECC_MAIN_UNLOCK();
	
	NFC_RnB_TRANS_DETECT_CLEAR();
	NFC_ILLEGAL_ACCESS_CLEAR();
	NFC_DECODING_DONE_CLEAR();
	NFC_ENCODING_DONE_CLEAR();
}

static void sdp_nand_enable_hwecc_ex_oob(struct mtd_info *mtd, int mode)
{
	NFECC_META_SELECT();
	NFECCCONF_REG |= NFECCCONF_META_CONV_SEL;// use custom conv value
	NFECCCONECC0_REG = 0x02f6792c;	// for 4-bit/24-byte META ECC
	NFECCCONECC1_REG = 0x002f564d;	// for 4-bit/24-byte META ECC
	NFECCCONF_REG &= ~0xFFF0;	
	NFECCCONF_REG |= 0x1730;

	/* calulate ecc */
	if (mode == NAND_ECC_WRITE) {
		NFECC_ENCODING();
	}
	/* correct ecc */
	else if (mode == NAND_ECC_READ) {
		NFECC_DECODING();
	}

	NFECC_MAIN_RESET();
	NFECC_MAIN_INIT();
	NFECC_MAIN_UNLOCK();
	
	NFC_RnB_TRANS_DETECT_CLEAR();
	NFC_ILLEGAL_ACCESS_CLEAR();
	NFC_DECODING_DONE_CLEAR();
	NFC_ENCODING_DONE_CLEAR();
}

/*
 * This function is called immediately after encoding ecc codes.
 * This function returns encoded ecc codes.
 * Written by jsgood
 */
static int sdp_nand_calculate_ecc_ex(struct mtd_info *mtd, const u_char *dat,
				  u_char *ecc_code)
{
	int i;
	
#ifdef CONFIG_NAND_SPL
	const uint32_t eccbyte = CONFIG_SYS_NAND_ECCBYTES;
#else
	const uint32_t eccbyte = mtd->ecclayout->eccbytes;	
#endif/*CONFIG_NAND_SPL*/
	const uint32_t eccbyte_align = eccbyte & ~0x3;
	/* Lock */
	NFECC_MAIN_LOCK();

#if 0
	while( !NFECC_IS_ENCODING_DONE() );
#endif
	ecc_code[0] = (NFECCPRGECCx_REG(0) >> (0)) & 0xFF;
	ecc_code[1] = (NFECCPRGECCx_REG(0) >> (8)) & 0xFF;
	ecc_code[2] = (NFECCPRGECCx_REG(0) >> (16)) & 0xFF;
	ecc_code[3] = (NFECCPRGECCx_REG(0) >> (24)) & 0xFF;
	ecc_code[4] = (NFECCPRGECCx_REG(1) >> (0)) & 0xFF;
	ecc_code[5] = (NFECCPRGECCx_REG(1) >> (8)) & 0xFF;
	ecc_code[6] = (NFECCPRGECCx_REG(1) >> (16)) & 0xFF;

#ifdef CONFIG_NAND_SDP_DEBUG
	printf("calculated ecc code ", ecc_code[0]);
	printf("%2x ", ecc_code[0]);
	printf("%2x ", ecc_code[1]);
	printf("%2x ", ecc_code[2]);
	printf("%2x ", ecc_code[3]);
	printf("%2x ", ecc_code[4]);
	printf("%2x ", ecc_code[5]);
	printf("%2x\n", ecc_code[6]);
#endif

#if 0
	/* 4byte write */
	for(i = 0; i < eccbyte_align; i+=4) {
		//*(unsigned long*)(ecc_code + i) = NFECCPRGECCx_REG(i);
		ecc_code[i] = (NFECCPRGECCx_REG(i) >> (i-0)) & 0xFF;
		ecc_code[i+1] = (NFECCPRGECCx_REG(i) >> (i+1)) & 0xFF;
		ecc_code[i+2] = (NFECCPRGECCx_REG(i) >> (i+2)) & 0xFF;
		ecc_code[i+3] = (NFECCPRGECCx_REG(i) >> (i+3)) & 0xFF;
	}

	/* redundant */
	for( ; i < eccbyte; i++) {
		ecc_code[i] = (NFECCPRGECCx_REG(i) >> (i-eccbyte_align)) & 0xFF;
	}
#endif
	NFECC_MAIN_UNLOCK();
	return 0;
}


/*
 * This function determines whether read data is good or not.
 * If SLC, must write ecc codes to controller before reading status bit.
 * If MLC, status bit is already set, so only reading is needed.
 * If status bit is good, return 0.
 * If correctable errors occured, do that.
 * If uncorrectable errors occured, return -1.
 * Written by jsgood
 */
static int sdp_nand_correct_data_ex(struct mtd_info *mtd, u_char *dat,
				 u_char *read_ecc, u_char *calc_ecc)
{
	struct nand_chip *this = mtd->priv;
	int i;
	int error_cnt, max_error, eccbyte;
	int err_loc, err_pat;

	eccbyte = this->ecc.bytes;

	/* write ecc data from spare*/
	this->write_buf(mtd, read_ecc, eccbyte);
	
	
	NFECC_MAIN_LOCK();

	while( !NFECC_IS_DECODING_DONE() );

	max_error = 4;
	error_cnt = NFECC_ERROR_COUNT();
	

	/* if bit7 is 1, no error */
	if( likely(error_cnt>>7 == 1) ) {
		return 0;
	}

	/* only error no masking */
	error_cnt &= 0x3F;
	

	/* Uncorrectable error */
	if(error_cnt > max_error) {
		printf("SDP NAND: ECC uncorrectable error detected. "
		       "Not correctable.\n");
		return -1;
	}

	/* error Correction */
	for(i = 0; i < error_cnt; i++) {
		/* NFECCERLx_REG to i-th ECC Error Byte Location(10bit) */
		err_loc = (NFECCERLx_REG(i >> 1) >> ((i&0x1) << 4)) & 0x3FF;
		/* NFECCERLx_REG to i-th ECC Error bit Pattern(8bit) */
		err_pat = (NFECCERPx_REG(i >> 2) >> ((i&3) << 3)) & 0xFF;

		printf("SDP NAND: 1 bit error detected at byte %ld. "
		       "Correcting from 0x%02x to 0x%02x...OK\n",
		       err_loc, dat[err_loc], dat[err_loc]^err_pat);
		
		dat[err_loc] ^= err_pat;
		
	}

	NFECC_MAIN_UNLOCK();
	return error_cnt;

}

#ifndef CONFIG_NAND_SPL
/**
 * nand_read_page_hwecc_oob_first - [REPLACABLE] hw ecc, read oob first
 * @mtd:	mtd info structure
 * @chip:	nand chip info structure
 * @buf:	buffer to store read data
 * @page:	page number to read
 *
 * Hardware ECC for large page chips, require OOB to be read first.
 * For this ECC mode, the write_page method is re-used from ECC_HW.
 * These methods read/write ECC from the OOB area, unlike the
 * ECC_HW_SYNDROME support with multiple ECC steps, follows the
 * "infix ECC" scheme and reads/writes ECC from the data area, by
 * overwriting the NAND manufacturer bad block markings.
 */
static int sdp_nand_read_page_hwecc_oob_first(struct mtd_info *mtd,
	struct nand_chip *chip, uint8_t *buf, int page)
{
	int i, eccsize = chip->ecc.size;
	int eccbytes = chip->ecc.bytes;
	int eccsteps = chip->ecc.steps;
	uint8_t *p = buf;
	uint8_t *ecc_code = chip->buffers->ecccode;
	uint32_t *eccpos = chip->ecc.layout->eccpos;
	uint8_t *ecc_calc = chip->buffers->ecccalc;
	const int block = ((struct sdp_nand_chip_data*)chip->priv)->cur_realpage >> (chip->phys_erase_shift - chip->page_shift);

	/* Read the OOB area first */
	chip->cmdfunc(mtd, NAND_CMD_READOOB, 0, page);
	chip->read_buf(mtd, chip->oob_poi, mtd->oobsize);

	/* if first block then eccpos start 0 */
	if( likely(block) ) {
		for (i = 0; i < chip->ecc.total; i++)
			ecc_code[i] = chip->oob_poi[eccpos[i]];
	}
	else {
		for (i = 0; i < chip->ecc.total; i++)
			ecc_code[i] = chip->oob_poi[i];

	}

#ifdef CONFIG_NAND_SDP_DEBUG
	printk("block %d, page %d\n", block, page);
	print_oob("sdp-nand read page oob", mtd);
#endif

	chip->cmdfunc(mtd, NAND_CMD_READ0, 0, page);
	for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {
		int stat;

		chip->ecc.hwctl(mtd, NAND_ECC_READ);
		chip->read_buf(mtd, p, eccsize); 

		stat = chip->ecc.correct(mtd, p, &ecc_code[i], NULL);
		if (stat < 0)
			mtd->ecc_stats.failed++;
		else
			mtd->ecc_stats.corrected += stat;

#ifdef CONFIG_NAND_SDP_DEBUG
		printk("debug: first 16bytes data\n");
		for(i = 0; i < 16; i++)
		{
			printk("%02x ", p[i]);
		}
		printk("\n");
#endif
	}

	return 0;
}

/**
 * nand_write_page_hwecc - [REPLACABLE] hardware ecc based page write function
 * @mtd:	mtd info structure
 * @chip:	nand chip info structure
 * @buf:	data buffer
 */
static void sdp_nand_write_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip,
				  const uint8_t *buf)
{
	int i, eccsize = chip->ecc.size;
	int eccbytes = chip->ecc.bytes;
	int eccsteps = chip->ecc.steps;
	uint8_t *ecc_calc = chip->buffers->ecccalc;
	const uint8_t *p = buf;
	uint32_t *eccpos = chip->ecc.layout->eccpos;
	const int block = ((struct sdp_nand_chip_data*)chip->priv)->cur_realpage >> (chip->phys_erase_shift - chip->page_shift);

	for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {
		chip->ecc.hwctl(mtd, NAND_ECC_WRITE);
		chip->write_buf(mtd, p, eccsize);
		chip->ecc.calculate(mtd, p, &ecc_calc[i]);
	}

	/* if first block then eccpos start 0 */
	if( likely(block) ) {
		for (i = 0; i < chip->ecc.total; i++)
			chip->oob_poi[eccpos[i]] = ecc_calc[i];
	}
	else {
		for (i = 0; i < chip->ecc.total; i++)
			chip->oob_poi[i] = ecc_calc[i];	
	}

	chip->write_buf(mtd, chip->oob_poi, mtd->oobsize);

	/*
	bug fix(20110530) : for block 0 ecc data erase.
	*/
	if(unlikely(block==0))
	{
		memset(chip->oob_poi, 0xFF, mtd->oobsize);
	}
}


/**
 * nand_read_oob_std - [REPLACABLE] the most common OOB data read function
 * @mtd:	mtd info structure
 * @chip:	nand chip info structure
 * @page:	page number to read
 * @sndcmd:	flag whether to issue read command or not
 */
static int sdp_nand_read_oob_hwecc(struct mtd_info *mtd, struct nand_chip *chip,
			     int page, int sndcmd)
{
	const uint8_t *buf = chip->oob_poi;
	const int length = mtd->oobsize;
	const int eccbytes = mtd->ecclayout->eccbytes;
	const int oobfree_off = mtd->ecclayout->oobfree[0].offset;
	const int oobfree_len = mtd->ecclayout->oobfree[0].length;
	int oob_pos = 0;
	int stat;
	
	if (sndcmd) {
		chip->cmdfunc(mtd, NAND_CMD_READOOB, 0, page);
		sndcmd = 0;
	}

	/* nand read befor oob data */
	chip->write_buf(mtd, buf+oob_pos, oobfree_off);
	oob_pos += oobfree_off;

	/* nand read oob data and oob data ecc */
	sdp_nand_enable_hwecc_ex_oob(mtd, NAND_ECC_READ);
	chip->write_buf(mtd, buf+oob_pos, oobfree_len + eccbytes);
	stat = sdp_nand_correct_data_ex(mtd, buf+oob_pos, buf+oob_pos+oobfree_len, NULL);
	oob_pos += oobfree_len + eccbytes;
	if (stat < 0)
		mtd->ecc_stats.failed++;
	else
		mtd->ecc_stats.corrected += stat;
	
	/* nand read and after oob data ecc */
	chip->write_buf(mtd, buf+oob_pos, length-oob_pos);

	return sndcmd;
}

/**
 * nand_write_oob_std - [REPLACABLE] the most common OOB data write function
 * @mtd:	mtd info structure
 * @chip:	nand chip info structure
 * @page:	page number to write

 0:1bit  valid bit
 1:24bit oob data
 25:7bit  oob data ecc
 32:28bit main data ecc
 60:4bit  not useed
 */
static int sdp_nand_write_oob_hwecc(struct mtd_info *mtd, struct nand_chip *chip,
			      int page)
{
	int status = 0;
	const uint8_t *buf = chip->oob_poi;
	const int length = mtd->oobsize;
	int oob_pos = 0;
	const int eccbytes = mtd->ecclayout->eccbytes;
	const int oobfree_off = mtd->ecclayout->oobfree[0].offset;
	const int oobfree_len = mtd->ecclayout->oobfree[0].length;
	uint8_t *oobecc = chip->oob_poi + oobfree_off+ oobfree_len;
	
	chip->cmdfunc(mtd, NAND_CMD_SEQIN, mtd->writesize, page);
	/* nand write befor oob data */
	chip->write_buf(mtd, buf+oob_pos, oobfree_off);
	oob_pos += oobfree_off;

	/* nand write oob data with ecc gen */
	sdp_nand_enable_hwecc_ex_oob(mtd, NAND_ECC_WRITE);
	chip->write_buf(mtd, buf+oob_pos, oobfree_len);
	sdp_nand_calculate_ecc_ex(mtd, buf+oob_pos/*oob data*/, oobecc);
	oob_pos += oobfree_len;
	
	/*write oob data ecc and after oob data */
	chip->write_buf(mtd, buf+oob_pos, length-oob_pos);

	/* Send command to program the OOB data */
	chip->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);

	status = chip->waitfunc(mtd, chip);

	return status & NAND_STATUS_FAIL ? -EIO : 0;
}
#endif/* !CONFIG_NAND_SPL */

#endif /* CONFIG_SYS_SDP_NAND_HWECC */

#ifndef __KERNEL__
/*
 * Board-specific NAND initialization. The following members of the
 * argument are board-specific (per include/linux/mtd/nand.h):
 * - IO_ADDR_R?: address to read the 8 I/O lines of the flash device
 * - IO_ADDR_W?: address to write the 8 I/O lines of the flash device
 * - hwcontrol: hardwarespecific function for accesing control-lines
 * - dev_ready: hardwarespecific function for  accesing device ready/busy line
 * - enable_hwecc?: function to enable (reset)  hardware ecc generator. Must
 *   only be provided if a hardware ECC is available
 * - eccmode: mode of ecc, see defines
 * - chip_delay: chip dependent delay for transfering data from array to
 *   read regs (tR)
 * - options: various chip options. They can partly be set to inform
 *   nand_scan about special functionality. See the defines for further
 *   explanation
 * Members with a "?" were not set in the merged testing-NAND branch,
 * so they are not set here either.
 */
int board_nand_init(struct nand_chip *nand)
{
	static int chip_n;

	if (chip_n >= MAX_CHIPS)
		return -ENODEV;

	//NAND Timing Set
	NF_TACLS(2);
	NF_TWRPH0(3);
	NF_TWRPH1(1);

	NFCONT_REG = (NFCONT_REG & ~NFCONT_WP) | NFCONT_ENABLE | NFCONT_nCE_ALL;

	nand->IO_ADDR_R		= (void __iomem *)NFDATA;
	nand->IO_ADDR_W		= (void __iomem *)NFDATA;
	nand->cmd_ctrl		= sdp_nand_hwcontrol;
	nand->dev_ready		= sdp_nand_device_ready;
	nand->select_chip	= sdp_nand_select_chip;
	nand->options		= 0;
#ifdef CONFIG_NAND_SPL
	nand->read_byte		= nand_read_byte;
	nand->write_buf		= nand_write_buf;
	nand->read_buf		= nand_read_buf;
#endif/*CONFIG_NAND_SPL*/

#ifdef CONFIG_SYS_SDP_NAND_HWECC
	nand->ecc.hwctl		= sdp_nand_enable_hwecc_ex;
	nand->ecc.calculate	= sdp_nand_calculate_ecc_ex;
	nand->ecc.correct	= sdp_nand_correct_data_ex;
	/*
	 * If you get more than 1 NAND-chip with different page-sizes on the
	 * board one day, it will get more complicated...
	 */
	nand->ecc.mode		= NAND_ECC_HW_OOB_FIRST;
	nand->ecc.size		= CONFIG_SYS_NAND_ECCSIZE;
	nand->ecc.bytes		= CONFIG_SYS_NAND_ECCBYTES;
#ifndef CONFIG_NAND_SPL
	nand->ecc.layout		= &sdp_oobinfo_64;
	nand->badblock_pattern	= &sdp_largepage_memorybased;
	nand->ecc.read_page		= sdp_nand_read_page_hwecc_oob_first;
	nand->ecc.write_page	= sdp_nand_write_page_hwecc;
	//nand->ecc.read_oob		= sdp_nand_read_oob_hwecc;
	//nand->ecc.write_oob		= sdp_nand_write_oob_hwecc;

#endif/*!CONFIG_NAND_SPL*/
#else
	nand->ecc.mode		= NAND_ECC_SOFT;
#endif /* CONFIG_SYS_SDP_NAND_HWECC */

	nand->priv		= &sdp_nand_data;
	sdp_nand_data.nand_cs = nand_cs[chip_n++];

	return 0;
}
#endif/* !__KERNEL */



#ifdef __KERNEL__
/* conversion functions */

inline static struct sdp_nand_mtd *sdp_nand_mtd_to_ours(struct mtd_info *mtd)
{
	return container_of(mtd, struct sdp_nand_mtd, mtd);
}

inline static struct sdp_nand_info *sdp_nand_mtd_to_info(struct mtd_info *mtd)
{
	return sdp_nand_mtd_to_ours(mtd)->sinfo;
}

inline static struct sdp_nand_info *to_nand_info(struct platform_device *pdev)
{
	return platform_get_drvdata(pdev);
}

inline static struct sdp_nand_platform *to_nand_plat(struct platform_device *pdev)
{
	return pdev->dev.platform_data;
}


#ifdef CONFIG_MTD_PARTITIONS
const char *part_probes[] = { "cmdlinepart", NULL };
static int sdp_nand_add_partition(struct sdp_nand_info *info,
				      struct sdp_nand_mtd *mtd)
{
	struct mtd_partition *part_info;
	int nr_part = 0;

//	if (set == NULL)
//		return add_mtd_device(&mtd->mtd);

	mtd->mtd.name = "sdp_nand0";
	nr_part = parse_mtd_partitions(&mtd->mtd, part_probes, &part_info, 0);

//	if (nr_part <= 0 && set->nr_partitions > 0) {
//		nr_part = set->nr_partitions;
//		part_info = set->partitions;
//	}

	if (nr_part > 0 && part_info)
		return add_mtd_partitions(&mtd->mtd, part_info, nr_part);

	return add_mtd_device(&mtd->mtd);
}
#else
static int sdp_nand_add_partition(struct sdp_nand_info *info,
				      struct sdp_nand_mtd *mtd)
{
	return add_mtd_device(&mtd->mtd);
}
#endif

inline static int sdp_nand_init_chip(struct sdp_nand_info *sinfo, struct sdp_nand_mtd *smtd)
{
	struct nand_chip * chip = &smtd->chip;
	static int chip_n;

	if (chip_n >= MAX_CHIPS)
		return -ENODEV;

	chip->IO_ADDR_R		= (void __iomem *)NFDATA;
	chip->IO_ADDR_W		= (void __iomem *)NFDATA;
	chip->cmd_ctrl		= sdp_nand_hwcontrol;
	chip->dev_ready		= sdp_nand_device_ready;
	chip->select_chip	= sdp_nand_select_chip;
	chip->options		= 0;


#ifdef CONFIG_SYS_SDP_NAND_HWECC
	/*
	 * If you get more than 1 NAND-chip with different page-sizes on the
	 * board one day, it will get more complicated...
	 */
	
	chip->ecc.mode		= NAND_ECC_HW_OOB_FIRST;
	chip->ecc.size		= 512;//CONFIG_SYS_NAND_ECCSIZE;
	chip->ecc.bytes		= 7;//CONFIG_SYS_NAND_ECCBYTES;
	dev_info(sinfo->dev, "enable H/W ECC. (MSGSize=%d, Bytes=%d)\n", chip->ecc.size, chip->ecc.bytes);

	chip->ecc.hwctl		= sdp_nand_enable_hwecc_ex;
	chip->ecc.calculate	= sdp_nand_calculate_ecc_ex;
	chip->ecc.correct	= sdp_nand_correct_data_ex;
	
	chip->ecc.layout		= &sdp_oobinfo_64;
	chip->badblock_pattern	= &sdp_largepage_memorybased;
	chip->ecc.read_page		= sdp_nand_read_page_hwecc_oob_first;
	chip->ecc.write_page	= sdp_nand_write_page_hwecc;
	//chip->ecc.read_oob		= sdp_nand_read_oob_hwecc;
	//chip->ecc.write_oob		= sdp_nand_write_oob_hwecc;
#else
	chip->ecc.mode		= NAND_ECC_SOFT;
#endif /* CONFIG_SYS_SDP_NAND_HWECC */

	chip->priv		= &sdp_nand_data;
	sdp_nand_data.nand_cs = nand_cs[chip_n++];

	return 0;
}

inline static int sdp_nand_init_hw(struct sdp_nand_info *sinfo)
{
	struct sdp_nand_platform *plat = sinfo->platform;
	//NAND Timing Set
	NF_TACLS(plat->tacls);
	NF_TWRPH0(plat->twrph0);
	NF_TWRPH1(plat->twrph1);
	NFCONT_REG = (NFCONT_REG & ~NFCONT_WP) | NFCONT_ENABLE | NFCONT_nCE_ALL;
	dev_info(sinfo->dev, "TACLS=%d, TWRPH0=%d, TWRPH1=%d\n", plat->tacls, plat->twrph0, plat->twrph1);
	dev_info(sinfo->dev, "enable NFCON and disalbe WP.\n");

	/* set 4bit ecc */
	NFECCCONF_REG |= 0x3/*main ecc bit*/ | 0x3<<4/*meta ecc bit*/ | 23<<8/*meta msg size*/ | 511<<16/*main msg size*/;

	dev_info(sinfo->dev, "NFCONF=%.8x, NFCONT=%.8x, NFSTAT=%.8x, NFECCCONF=%.8x, NFECCCONT=%.8x, NFECCSTAT=%.8x\n",
		NFCONF_REG, NFCONT_REG, NFSTAT_REG, NFECCCONF_REG, NFECCCONT_REG, NFECCSTAT_REG);
	return 0;
}



/* device management functions */
static int sdp_nand_remove(struct platform_device *pdev)
{
	struct sdp_nand_info *sinfo = to_nand_info(pdev);

	platform_set_drvdata(pdev, NULL);

	if(sinfo == NULL)
		return 0;

	/* free mtds and chip */
	if(sinfo->mtds != NULL)
	{
		struct sdp_nand_mtd *smtd = sinfo->mtds;
		int i;
		for(i = 0; i < sinfo->mtd_count; i++, smtd++)
		{
			dev_dbg(&pdev->dev, "free mtd %d (%p)\n", i, smtd);
			nand_release(&smtd->mtd);
		}
		kfree(sinfo->mtds);
	}

	/* free the common resources */
	if (sinfo->clk != NULL && !IS_ERR(sinfo->clk)) {
	//	if (!allow_clk_stop(sinfo))
		clk_disable(sinfo->clk);
		clk_put(sinfo->clk);
	}

	if (sinfo->regbase != NULL) {
		iounmap(sinfo->regbase);
		sinfo->regbase = NULL;
	}

	if (sinfo->area != NULL) {
		release_mem_region(sinfo->area->start, resource_size(sinfo->area));
		sinfo->area = NULL;
	}

	if(sinfo->res)
		sinfo->res = NULL;

	kfree(sinfo);
	
	return 0;
}

static int sdp_nand_probe(struct platform_device *pdev)
{
	struct sdp_nand_info *sinfo;
	struct sdp_nand_platform *splat = to_nand_plat(pdev);
	struct resource *res;
	int err = 0;
	int i = 0;
	int ret = 0;
	
	dev_info(&pdev->dev, "new sdp-nand device probed.\n");

	sinfo = kzalloc(sizeof(*sinfo), GFP_KERNEL);
	if(sinfo == NULL)
	{
		dev_err(&pdev->dev, "no memory for nand info\n");
		err = -ENOMEM;
		goto exit_error;
	}

	platform_set_drvdata(pdev, sinfo);
	

	spin_lock_init(&sinfo->controller.lock);
	init_waitqueue_head(&sinfo->controller.wq);

	/* get the clock source and enable clock */
	sinfo->clk = clk_get(&pdev->dev, "PCLK");
	if (IS_ERR(sinfo->clk)) {
		dev_err(&pdev->dev, "failed to get clock\n");
		err = -ENOENT;
		goto exit_error;
	}
	clk_enable(sinfo->clk);

	/* allocate and map the resource */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "unable to get iomem resource\n");
		err = -ENODEV;
		goto exit_error;
	}

	sinfo->area= request_mem_region(res->start, resource_size(res), pdev->name);
	if (!sinfo->area) {
		dev_err(&pdev->dev, "unable to request iomem resources\n");
		err = -EBUSY;
		goto exit_error;
	}

	sinfo->regbase = ioremap(res->start, resource_size(res));
	if (!sinfo->regbase) {
		dev_err(&pdev->dev, "failed to map resources\n");
		err = -ENODEV;
		goto exit_error;
	}
	/* for using #define NFCONF			(SDP_NAND_BASE + NFCONF_OFFSET) */
	_sdp_nand_base = sinfo->regbase;

	sinfo->dev     = &pdev->dev;
	sinfo->platform = splat;

	dev_dbg(&pdev->dev, "mapped registers at %p\n", sinfo->regbase);

	/* initialise the hardware */
	sdp_nand_init_hw(sinfo);
	
	/* allocate our information */
	sinfo->mtd_count = 1;//just one mtd support
	sinfo->mtds = kzalloc(sinfo->mtd_count * sizeof(*sinfo->mtds), GFP_KERNEL);
	if (sinfo->mtds == NULL) {
		dev_err(&pdev->dev, "failed to allocate mtd storage\n");
		err = -ENOMEM;
		goto exit_error;
	}
	
	/* initialise all possible chips */
	struct sdp_nand_mtd *smtd = sinfo->mtds;
	for(i = 0; i < sinfo->mtd_count; i++, smtd++)
	{
		sdp_nand_init_chip(sinfo, smtd);
		smtd->mtd.priv = &smtd->chip;
		smtd->scan_result = nand_scan_ident(&smtd->mtd, 1/*MAX_CHIPS*/, NULL);
		if(smtd->scan_result == 0)
		{
			nand_scan_tail(&smtd->mtd);
			sdp_nand_add_partition(sinfo, smtd);
		}
	}

	dev_info(&pdev->dev, "initialised all ok.(iomem=%p)\n", sinfo->regbase);
	return 0;	
	
exit_error:
	sdp_nand_remove(pdev);
	return err==0 ? -EINVAL : err;

}




static struct platform_driver sdp1004_nand_driver = {
	.probe		= sdp_nand_probe,
	.remove		= sdp_nand_remove,
//	.suspend	= sdp_nand_suspend,
//	.resume		= sdp_nand_resume,
	.driver		= {
		.name	= "sdp-nand",
		.owner	= THIS_MODULE,
	},
};

static int __init sdp_nand_init(void)
{
	printk("init : SDP NAND Driver, (c) 2011 SoC Group.\n");

	return platform_driver_register(&sdp1004_nand_driver);
}

static void __exit sdp_nand_exit(void)
{
	platform_driver_unregister(&sdp1004_nand_driver);
}

module_init(sdp_nand_init);
module_exit(sdp_nand_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dongseok Lee <drain.lee@samsung.com>");
MODULE_DESCRIPTION("SDP1xxx MTD NAND driver");
MODULE_ALIAS("platform:sdp-nand");
#endif/* __LINUX_OS__ */

