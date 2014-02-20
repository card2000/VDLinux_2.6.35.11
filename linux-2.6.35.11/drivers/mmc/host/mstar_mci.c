////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2011 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// ("MStar Confidential Information") by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////

#include <linux/module.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/blkdev.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/dma-mapping.h>
#include <linux/mmc/host.h>
#include <linux/scatterlist.h>
#ifdef CONFIG_MSTAR_AMBER3
#include <mstar/mstar_chip.h>
#endif
#include "chip_int.h"
#include "mstar_mci.h"

/******************************************************************************
 * Defines
 ******************************************************************************/
#define DRIVER_NAME					"mstar_mci"
#define FL_SENT_COMMAND				(1 << 0)
#define FL_SENT_STOP				(1 << 1)
#define LOOP_DELAY_WAIT_COMPLETE	(3000000)

#define MSTAR_MCI_IRQ               0

/* For debugging purpose */
#define MSTAR_MCI_DEBUG				0

#define CMD_ARG_DEBUG               0
#define CMD_RSP_DEBUG               0

#ifdef CONFIG_MSTAR_AMBER3

/* For clock setting */
#define CLK_48M                     0
#if !(defined(CLK_48M) && CLK_48M)
#define CLK_43M                     0
#if !(defined(CLK_43M) && CLK_43M)
#define CLK_40M                     0
#if !(defined(CLK_40M) && CLK_40M)
#define CLK_36M                     0
#if !(defined(CLK_36M) && CLK_36M)
#define CLK_32M                     1
#if !(defined(CLK_32M) && CLK_32M)
#define CLK_27M                     0
#if !(defined(CLK_27M) && CLK_27M)
#define CLK_20M                     0
#endif  // CLK_27M
#endif  // CLK_32M
#endif  // CLK_36M
#endif  // CLK_40M
#endif  // CLK_43M
#endif  // CLK_48M

/* For transfer mode setting */
#define BYPASS_MODE_ENABLE          1
#define SDR_MODE_ENABLE             0
#define DDR_MODE_ENABLE             0

#endif  // CONFIG_MSTAR_AMBER3

#define MMC_SPEED_TEST              0

#if defined(MMC_SPEED_TEST) && MMC_SPEED_TEST

#define DMA_TIME_TEST               0
#if DMA_TIME_TEST
#define DMA_TIME_TEST_READ          0
#endif

#define REQ_TIME_TEST               0

#define CMD_TIME_TEST               0

#if defined(CMD_TIME_TEST) && CMD_TIME_TEST
#define TOG_TIME_TEST               0
#endif

#endif

#if (MSTAR_MCI_DEBUG)
#undef pr_debug
#define pr_debug(fmt, arg...)		printk(KERN_INFO fmt, ##arg)
#else
#undef pr_debug
#define pr_debug(fmt, arg...)
#endif
#define simple_debug(fmt, arg...)	//printk(KERN_INFO fmt, ##arg)

/******************************************************************************
 * Function Prototypes
 ******************************************************************************/
static void mstar_mci_process_next(struct mstar_mci_host *pHost_st);
static void mstar_mci_completed_command(struct mstar_mci_host *pHost_st);

static void mstar_mci_request(struct mmc_host *pMMC_st, struct mmc_request *pMRQ_st);
static void mstar_mci_set_ios(struct mmc_host *pMMC_st, struct mmc_ios *pIOS_st);
static s32 mstar_mci_get_ro(struct mmc_host *pMMC_st);

static void fcie_reg_dump(struct mstar_mci_host *pHost_st);

/*****************************************************************************
 * Define Static Global Variables
 ******************************************************************************/

/* MSTAR Multimedia Card Interface Operations */
static const struct mmc_host_ops sg_mstar_mci_ops =
{
    .request =	mstar_mci_request,
    .set_ios =	mstar_mci_set_ios,
    .get_ro =	mstar_mci_get_ro,
};

static struct resource sg_mmc_resources_st[] =
{
    [0] =
    {
        .start =	FCIE0_BASE,
        .end =		FCIE0_BASE + (0x00000400*2) - 1,
        .flags =	IORESOURCE_MEM,
    },
    [1] =
    {
        .start =    E_IRQ_NFIE,
        .end =      E_IRQ_NFIE,
        .flags =    IORESOURCE_IRQ,
    },
};

static struct platform_device sg_mstar_mmc_device_st =
{
    .name =	"mstar_mci",
    .id = 0,
    .resource =	sg_mmc_resources_st,
    .num_resources = ARRAY_SIZE(sg_mmc_resources_st),
};

/******************************************************************************
 * Functions
 ******************************************************************************/
#if defined(DMA_TIME_TEST) && DMA_TIME_TEST
#if DMA_TIME_TEST_READ
static u32 totalreadlen = 0;
static u32 totalreadtime = 0;
#else
static u32 totalwritelen = 0;
static u32 totalwritetime = 0;
#endif
#endif

#if defined(REQ_TIME_TEST) && REQ_TIME_TEST
static u32 totalreqlen = 0;
static u32 totalreqtime = 0;
#endif

#if defined(CMD_TIME_TEST) && CMD_TIME_TEST
static u32 totalcmdlen = 0;
static u32 totalcmdtime = 0;
#endif

#if defined(TOG_TIME_TEST) && TOG_TIME_TEST
static u32 totalregtime = 0;
#endif

#if defined(MMC_SPEED_TEST) && MMC_SPEED_TEST
void mstar_mci_PIU_Timer1_Start(void)
{
    // Reset PIU Timer1
    reg_writel(0xFFFF, TIMER1_MAX_LOW);
    reg_writel(0xFFFF, TIMER1_MAX_HIGH);
    reg_writel(0, TIMER1_ENABLE);

    // Start PIU Timer1
    reg_writel(0x1, TIMER1_ENABLE);
}

u32 mstar_mci_PIU_Timer1_End(void)
{
    u32 u32HWTimer = 0;
    u32 u32TimerLow = 0;
    u32 u32TimerHigh = 0;

    // Get timer value
    u32TimerLow = reg_readl(TIMER1_CAP_LOW);
    u32TimerHigh = reg_readl(TIMER1_CAP_HIGH);

    u32HWTimer = (u32TimerHigh<<16) | u32TimerLow;

    return u32HWTimer;
}
#endif

static u8 waitD0High(struct mstar_mci_host *pHost_st, u32 u32LoopCnt)
{
    u32 u32Cnt=0;

    while ((fcie_reg_read(pHost_st, FCIE_SD_STS) & BIT8) == 0) /* card busy */
    {
        if ((u32Cnt++) == u32LoopCnt)
        {
        	printk("mstar_mci: card busy\n");
            return 1;
        }
        udelay(1);
    }
    return 0;
}

static u8 clear_fcie_status(struct mstar_mci_host *pHost_st, u16 reg, u16 value)
{
   u8 i=0;

   do
   {
       fcie_reg_write(pHost_st, reg, value);
       if (++i == 0)
       {
           printk("mstar_mci: Error, clear FCIE status in RIU time out!\n");
           return 1;
       }
   } while(fcie_reg_read(pHost_st, reg)&value);

   return 0;
}

static u8 wait_fifoclk_ready(struct mstar_mci_host *pHost_st)
{
   u16 wait_cnt=0;

   do
   {
       if (++wait_cnt == 0)
       {
           printk("mstar_mci: Error, wait FIFO clock ready time out!\n");
           return 1;
       }
   } while((fcie_reg_read(pHost_st, FCIE_MMA_PRI_REG)&FIFO_CLKRDY)==0);

   return 0;
}

#if defined(MSTAR_MCI_IRQ) && MSTAR_MCI_IRQ
static DECLARE_WAIT_QUEUE_HEAD(fcie_wait);
static atomic_t fcie_int = ATOMIC_INIT(0);
#endif

static u32 mstar_mci_wait_event(struct mstar_mci_host *pHost_st, u32 u32Event, u32 u32Timeout)
{
    u32 i = 0;

    #if defined(MSTAR_MCI_IRQ) && MSTAR_MCI_IRQ

    if( wait_event_timeout(fcie_wait, (atomic_read(&fcie_int) == 1), u32Timeout/HZ) == 0 )
    {
        printk("\033[31m Warning!!! There is an interrupt lost!!!\033[m\n");

        // Error handling of interrupt lose
        for(i=0; i<u32Timeout; i++)
        {
            if( (fcie_reg_read(pHost_st, FCIE_MIE_EVENT) & u32Event) == u32Event )
                break;

            udelay(1);
        }

    }

    atomic_set(&fcie_int, 0);

    #else

    for(i=0; i<u32Timeout; i++)
    {
        if( (fcie_reg_read(pHost_st, FCIE_MIE_EVENT) & u32Event) == u32Event )
            break;

        udelay(1);
    }

    #endif

    if( i == u32Timeout )
    {
        printk("\033[31mWait event timeout!\033[m\n");
        fcie_reg_dump(pHost_st);
        // We don't hang the system when timeout now
    }

    fcie_reg_write(pHost_st, FCIE_MIE_EVENT, u32Event);

    return i;

}

static void fcie_reg_dump(struct mstar_mci_host *pHost_st)
{
	u16 i;
	u16 regval;

	printk("\nCHIPTOP:\n");
	printk("allpadin:\t\t%04X\n", fcie_readw(REG_ALLAD_IN));
    printk("USB DRV BUS config: \t%04X\n", fcie_readw(REG_USB_DRV_BUS_CONFIG));
    printk("EXT INT: \t\t%04X\n", fcie_readw(REG_EXTINT));
    printk("SD pad: \t\t\t%04X\n", fcie_readw(REG_SD_CONFIG));
	printk("PCM pad:\t\t\t%04X\n", fcie_readw(REG_PCM_CONFIG));
	printk("eMMC pad:\t\t%04X\n", fcie_readw(REG_EMMC_CONFIG));
	printk("nand pad:\t\t%04X\n", fcie_readw(REG_NAND_CONFIG));

	printk("\nCLKGEN0:\n");
	printk("fcie clk:\t\t%04X\n", fcie_readw(REG_CLK_NFIE));

	printk("\nDump FCIE3 bank:\n");

	regval = fcie_reg_read(pHost_st, FCIE_MIE_EVENT);
	printk("MIE_EVENT(0x00)=\t\t%04X ", regval);
	if( regval & MMA_DATA_END )
		printk("\033[31mMMA_DATA_END\033[m ");
	if( regval & SD_CMD_END )
		printk("\033[31mSD_CMD_END\033[m ");
	if( regval & SD_DATA_END )
		printk("\033[31mSD_DATA_END\033[m ");
	if( regval & CARD_DMA_END )
		printk("\033[31mCARD_DMA_END\033[m ");
	if( regval & MMA_LAST_DONE)
		printk("\033[31mMMA_LAST_DONE\033[m ");
	printk("\n");

	regval = fcie_reg_read(pHost_st, FCIE_MIE_INT_EN);
	printk("MIE_INT_EN(0x01)=\t%04X ", regval);
	if( regval & MMA_DATA_END )
		printk("\033[31mMMA_DATA_END\033[m ");
	if( regval & SD_CMD_END )
		printk("\033[31mSD_CMD_END\033[m ");
	if( regval & SD_DATA_END )
		printk("\033[31mSD_DATA_END\033[m ");
	if( regval & CARD_DMA_END )
		printk("\033[31mCARD_DMA_END\033[m ");
	if( regval & MMA_LAST_DONE)
		printk("\033[31mMMA_LAST_DONE\033[m ");
	printk("\n");

	regval = fcie_reg_read(pHost_st, FCIE_MMA_PRI_REG);
	printk("MMA_PRI_REG(0x02)=\t%04X ", regval);
	if( regval & JOB_RW_DIR)
		printk("\033[31mWrite\033[m ");
	else
		printk("\033[31mRead\033[m ");
	if( regval & FIFO_CLKRDY )
		printk("\033[31mFIFO_CLKRDY\033[m ");
	printk("\n");

	regval = fcie_reg_read(pHost_st, FCIE_MIU_DMA1);
	printk("MIU_DMA1(0x03)=\t\t%04X ", regval);
	if( regval & MIU1_SELECT )
		printk("\033[31mMIU1\033[m\n");
	else
		printk("\033[31mMIU0\033[m\n");

	regval = fcie_reg_read(pHost_st, FCIE_CARD_EVENT);
	printk("CARD_EVENT(0x05)=\t%04X ", regval);
	if( regval & SD_STS_CHG )
		printk("\033[31mSD_STS_CHG\033m");
	printk("\n");

	regval = fcie_reg_read(pHost_st, FCIE_CARD_INT_EN);
	printk("CARD_INT_EN(0x06)=\t%04X ", regval);
	if( regval & SD_STS_EN )
		printk("\033[31mSD_STS_EN\033[m");
	printk("\n");

	regval = fcie_reg_read(pHost_st, FCIE_CARD_DET);
	printk("CARD_DET(0x07)=\t\t%04X ", regval);
	if( regval & SD_DET_N )
		printk("\033[31mSD_DET_N\033[m ");
	printk("\n");

	regval = fcie_reg_read(pHost_st, FCIE_MIE_PATH_CTL);
	printk("MIE_PATH_CTL(0x0A)=\t%04X ", regval);
	if( regval & MMA_ENABLE )
		printk("\033[31mMMA_ENABLE\033[m ");
	if( regval & SD_EN )
		printk("\033[31mSD_EN\033[m ");
	if( regval & NC_EN )
		printk("\033[31mNC_EN\033[m ");
	printk("\n");

	printk("JOB_BL_CNT(0x0B)=\t%04X\n", fcie_reg_read(pHost_st, FCIE_JOB_BL_CNT));
	printk("TR_BL_CNT(0x0C)=\t\t%04X\n", fcie_reg_read(pHost_st, FCIE_TR_BL_CNT));

    fcie_setbits(pHost_st, FCIE_JOB_BL_CNT, BIT15);
    fcie_setbits(pHost_st, FCIE_JOB_BL_CNT, BIT14);
    printk("\tMIU TR_BL_CNT(0x0C)=\t%04X\n", fcie_reg_read(pHost_st, FCIE_TR_BL_CNT));

    fcie_clrbits(pHost_st, FCIE_JOB_BL_CNT, BIT14);
    printk("\tCARD TR_BL_CNT(0x0C)=\t%04X\n", fcie_reg_read(pHost_st, FCIE_TR_BL_CNT));

    fcie_clrbits(pHost_st, FCIE_JOB_BL_CNT, BIT15);

	printk("RSP_SIZE(0x0D)=\t\t%04X\n", fcie_reg_read(pHost_st, FCIE_RSP_SIZE));
	printk("CMD_SIZE(0x0E)=\t\t%04X\n", fcie_reg_read(pHost_st, FCIE_CMD_SIZE));
	printk("CARD_WD_CNT(0x0F)=\t%04X\n", fcie_reg_read(pHost_st, FCIE_CARD_WD_CNT));

	regval = fcie_reg_read(pHost_st, FCIE_SD_MODE);
	printk("SD_MODE(0x10)=\t\t%04X ", regval);
	if( regval & SD_CLK_EN )
		printk("\033[31mSD_CLK_EN\033[m ");
	if( regval & SD_DAT_LINE0 )
		printk("\033[31mSD_DAT_LINE0\033[m ");
	if( regval & SD_DAT_LINE1 )
		printk("\033[31mSD_DAT_LINE1\033[m ");
	if( regval & SD_CS_EN )
		printk("\033[31mSD_CS_EN\033[m ");
	if( regval & SD_DEST )
		printk("\033[31mCIFC\033[m ");
	else
		printk("\033[31mCIFD\033[m ");
	if( regval & SD_DATSYNC )
		printk("\033[31mSD_DATSYNC\033[m ");
	if( regval & SDIO_SD_BUS_SW )
		printk("\033[31mSDIO_SD_BUS_SW!!!\033[m ");
    if( regval & SD_DMA_RD_CLK_STOP )
		printk("\033[31mSD_DMA_RD_CLK_STOP!!!\033[m ");
	printk("\n");

	regval = fcie_reg_read(pHost_st, FCIE_SD_CTL);
	printk("SD_CTL(0x11)=\t\t%04X ", regval);
	if( regval & SD_RSPR2_EN )
		printk("\033[31mSD_RSPR2_EN\033[m ");
	if( regval & SD_RSP_EN )
		printk("\033[31mSD_RSP_EN\033[m ");
	if( regval & SD_CMD_EN )
		printk("\033[31mSD_CMD_EN\033[m ");
	if( regval & SD_DTRX_EN )
		printk("\033[31mSD_DTRX_EN\033[m ");
	if( regval & SD_DTRX_DIR )
		printk("\033[31mSD_DTRX_DIR\033[m ");
	printk("\n");

	regval = fcie_reg_read(pHost_st, FCIE_SD_STS);
	printk("SD_STS(0x12)=\t\t%04X ", regval);
	if( regval & SD_DAT_CERR )
		printk("\033[31mSD_DAT_CERR\033[m ");
	if( regval & SD_DAT_STSERR )
		printk("\033[31mSD_DAT_STSERR\033[m ");
	if( regval & SD_DAT_STSNEG )
		printk("\033[31mSD_DAT_STSNEG\033[m ");
	if( regval & SD_DAT_NORSP )
		printk("\033[31mSD_DAT_NORSP\033[m ");
	if( regval & SD_CMDRSP_CERR )
		printk("\033[31mSD_CMDRSP_CERR\033[m ");
    if( regval & SD_WR_PRO_N )
		printk("\033[31mSD_WR_PRO_N\033[m ");
    if( regval & SD_CARD_BUSY )
		printk("\033[31mSD_CARD_BUSY\033[m ");
	printk("\n");

	regval = fcie_reg_read(pHost_st, SDIO_BLK_SIZE);
	printk("SDIO_CTL(0x1B)=\t\t%04X ", regval);
	if( regval & SDIO_BLK_MOD_EN )
		printk("\033[31mSDIO_BLK_MOD_EN\033[m ");
	printk("\n");

	printk("SDIO_MIU_DMA0(0x1C)=\t%04X\n", fcie_reg_read(pHost_st, SDIO_MIU_DMA0));
	printk("SDIO_MIU_DMA1(0x1D)=\t%04X\n", fcie_reg_read(pHost_st, SDIO_MIU_DMA1));
	printk("MIU_OFFSET(0x2E)=\t%04X\n", fcie_reg_read(pHost_st, FCIE_DMA_OFFSET));

	regval = fcie_reg_read(pHost_st, EMMC_BOOT_CONFIG);
	printk("EMMC_BOOT_CONFIG(0x2F)=\t%04X ", regval);
	if( regval & BOOT_STG2_EN )
		printk("\033[31mBOOT_STG2_EN\033[m ");
	if( regval & BOOT_END_EN )
		printk("\033[31mBOOT_END_EN\033[m ");
	printk("\n");

	printk("TEST_MODE(0x30)=\t\t%04X\n", fcie_reg_read(pHost_st, FCIE_TEST_MODE));

    #ifdef CONFIG_MSTAR_AMBER3
    printk("SD_DDR_CNT(0x33)=\t\t%04X\n", fcie_reg_read(pHost_st, FCIE_SD_TOGGLE_CNT));

    regval = fcie_reg_read(pHost_st, FCIE_MISC);
    printk("MISC(0x36)=\t\t\t%04X ", regval);
    if( regval & DQS_MODE_DELEY_2T )
        printk("\033[31mDQS_MODE_DELEY_2T\033[m ");
    if( regval & DQS_MODE_DELAY_1_5T )
        printk("\033[DQS_MODE_DELAY_1_5T\033[m ");
    if( regval & DQS_MODE_DELAY_2_5T )
        printk("\033[DQS_MODE_DELAY_2_5T\033[m ");
    if( regval & DQS_MODE_DELAY_1T )
        printk("\033[DQS_MODE_DELAY_NO_DELAY\033[m ");
    #endif

	printk("\nCIFC:\n");
	for(i=0x60; i<0x80; i++)
    {
        if(i%8==0) printk("%02X: ", (i/8)*8);
        printk("0x%04X ", fcie_reg_read(pHost_st, i*4) );
        if(i%8==7) printk(":%02X\r\n", ((i/8)*8)+7);
    }

	printk("\nDump FCIE3 debug ports:\n");
	for(i=0; i<=7; i++)
	{
		// Enable debug ports
		fcie_clrbits(pHost_st, FCIE_TEST_MODE, (BIT10|BIT9|BIT8));
		fcie_setbits(pHost_st, FCIE_TEST_MODE, i<<8);

		// Dump debug ports
		printk("\nDebug Mode \033[31m%d:\033[m\n", i);
		printk("DBus[15:0]=\033[34m%04X\033[m\n", fcie_reg_read(pHost_st, FCIE_DEBUG_BUS_LOW));
		printk("DBus[23:16]=\033[34m%04X\033[m\n", fcie_reg_read(pHost_st, FCIE_DEBUG_BUS_HIGH));
	}

}

#ifdef CONFIG_MSTAR_AMBER3
static void mstar_mci_disable_stop_clock(struct mstar_mci_host *pHost_st)
{
    fcie_reg_write(pHost_st, FCIE_DEBUG_BUS_LOW, 0x4000);
}

void mstar_mci_reset_toggle_count(struct mstar_mci_host *pHost_st)
{
    fcie_reg_write(pHost_st, FCIE_DEBUG_BUS_LOW, 0x8000);
    udelay(1);
    fcie_reg_write(pHost_st, FCIE_DEBUG_BUS_LOW, 0x0000);
}

static void mstar_mci_set_toggle_count(struct mstar_mci_host *pHost_st)
{
    if( pHost_st->cmd->data->flags == MMC_DATA_READ )
        fcie_reg_write(pHost_st, FCIE_SD_TOGGLE_CNT, 0x111);
    else
        fcie_reg_write(pHost_st, FCIE_SD_TOGGLE_CNT, 0x11A);
}

static void mstar_mci_config_ddr_mode(struct mstar_mci_host *pHost_st)
{
    fcie_writew(0x0080, REG_EMMC_CONFIG); // Switch pad for eMMC DDR
    fcie_reg_write(pHost_st, EMMC_BOOT_CONFIG, 0x0300); // DDR
    fcie_reg_write(pHost_st, FCIE_MISC, DQS_MODE_DELAY_2_5T);
}
#endif

static void mstar_mci_pre_dma_read(struct mstar_mci_host *pHost_st)
{
    /* Define Local Variables */
    struct scatterlist *pSG_st = 0;
    struct mmc_command *pCmd_st = 0;
    struct mmc_data *pData_st = 0;

    int sglen = 0;
    u32 dmalen = 0;

    dma_addr_t dmaaddr = 0;

    /* Parameter out-of-bound check */
    if (!pHost_st)
    {
        printk(KERN_WARNING "mstar_mci_pre_dma_read parameter pHost_st is NULL\n");
        return;
    }
    /* End Parameter out-of-bound check */

    pr_debug("pre dma read\n");

    pCmd_st = pHost_st->cmd;
    if (!pCmd_st)
    {
        pr_debug("no command\n");
        return;
    }

    pData_st = pCmd_st->data;
    if (!pData_st)
    {
        pr_debug("no data\n");
        return;
    }

    sglen = dma_map_sg(mmc_dev(pHost_st->mmc), pData_st->sg, pData_st->sg_len, DMA_FROM_DEVICE);

    pSG_st = &pData_st->sg[0];

    dmaaddr = (u32)sg_dma_address(pSG_st);
    dmalen = sg_dma_len(pSG_st);

    #ifdef CONFIG_MSTAR_AMBER3
    if( dmaaddr >= MSTAR_MIU1_BUS_BASE)
    {
        dmaaddr -= MSTAR_MIU1_BUS_BASE;
        fcie_setbits(pHost_st, FCIE_MIU_DMA1, MIU1_SELECT);
    }
    else
    {
        dmaaddr -= MSTAR_MIU0_BUS_BASE;
        fcie_clrbits(pHost_st, FCIE_MIU_DMA1, MIU1_SELECT);
    }
    #endif

    fcie_reg_write(pHost_st,FCIE_JOB_BL_CNT,(dmalen/512));

    fcie_reg_write(pHost_st,SDIO_MIU_DMA0,(((u32)dmaaddr) & 0xFFFF));
    fcie_reg_write(pHost_st,SDIO_MIU_DMA1,(((u32)dmaaddr) >> 16));

    pr_debug("pre dma read done\n");
}

static void mstar_mci_post_dma_read(struct mstar_mci_host *pHost_st)
{
    /* Define Local Variables */
    struct mmc_command	*pCmd_st = 0;
    struct mmc_data		*pData_st = 0;
    struct scatterlist	*pSG_st = 0;

    int i;
    u32 dmalen = 0;

    dma_addr_t dmaaddr = 0;

    /* Parameter out-of-bound check */
    if (!pHost_st)
    {
        printk(KERN_WARNING "mstar_mci_post_dma_read parameter pHost_st is NULL\n");
        return;
    }
    /* End Parameter out-of-bound check */

    pr_debug("post dma read\n");

    pCmd_st = pHost_st->cmd;

    if (!pCmd_st)
    {
        pr_debug("no command\n");
        return;
    }

    pData_st = pCmd_st->data;
    if (!pData_st)
    {
        pr_debug("no data\n");
        return;
    }

    pSG_st = &(pData_st->sg[0]);
    pData_st->bytes_xfered += pSG_st->length;

    for(i=1; i<pData_st->sg_len; i++)
    {
        pSG_st = &(pData_st->sg[i]);

        dmaaddr = sg_dma_address(pSG_st);
        dmalen = sg_dma_len(pSG_st);

        #ifdef CONFIG_MSTAR_AMBER3
        if( dmaaddr >= MSTAR_MIU1_BUS_BASE)
        {
            dmaaddr -= MSTAR_MIU1_BUS_BASE;
            fcie_setbits(pHost_st, FCIE_MIU_DMA1, MIU1_SELECT);
        }
        else
        {
            dmaaddr -= MSTAR_MIU0_BUS_BASE;
            fcie_clrbits(pHost_st, FCIE_MIU_DMA1, MIU1_SELECT);
        }
        #endif

        fcie_reg_write(pHost_st,FCIE_JOB_BL_CNT,(dmalen/512));

        fcie_reg_write(pHost_st,SDIO_MIU_DMA0,(((u32)dmaaddr) & 0xFFFF));
        fcie_reg_write(pHost_st,SDIO_MIU_DMA1,(((u32)dmaaddr) >> 16));

        //wait clock sync
        udelay(1);

        wait_fifoclk_ready(pHost_st);

        #if defined(MSTAR_MCI_IRQ) && MSTAR_MCI_IRQ
        fcie_setbits(pHost_st, FCIE_MIE_INT_EN, MMA_LAST_DONE);
        #endif

        fcie_setbits(pHost_st, FCIE_MIE_PATH_CTL, MMA_ENABLE);

        fcie_reg_write(pHost_st, FCIE_SD_CTL, SD_DTRX_EN);

        if( mstar_mci_wait_event(pHost_st, MMA_DATA_END|SD_DATA_END|CARD_DMA_END|MMA_LAST_DONE, LOOP_DELAY_WAIT_COMPLETE) == LOOP_DELAY_WAIT_COMPLETE )
        {
            printk("P: Wait MMA last done timeout!\n");
            while(1);
        }

        pData_st->bytes_xfered += pSG_st->length;
    }

    dma_unmap_sg(mmc_dev(pHost_st->mmc), pData_st->sg, pData_st->sg_len, DMA_FROM_DEVICE);

}

static s32 mstar_mci_handle_cmdrdy(struct mstar_mci_host *pHost_st)
{
    struct mmc_command	*pCmd_st = 0;
    struct mmc_data		*pData_st = 0;
    struct scatterlist	*pSG_st = 0;

    int i;
    int sglen;
    u32 dmalen = 0;

    dma_addr_t dmaaddr = 0;

    #if defined(DMA_TIME_TEST) && DMA_TIME_TEST
    #if !(defined(DMA_TIME_TEST_READ) && DMA_TIME_TEST_READ)
    u32 u32Ticks = 0;
    #endif
    #endif

    /* Parameter out-of-bound check */
    if (!pHost_st)
    {
        printk(KERN_WARNING "mstar_mci_handle_cmdrdy parameter pHost_st is NULL\n");
        return -EINVAL;
    }
    /* End Parameter out-of-bound check */

    if (!pHost_st->cmd)
    {
        return 1;
    }
    pCmd_st = pHost_st->cmd;

    if (!pCmd_st->data)
    {
        return 1;
    }
    pData_st = pCmd_st->data;

    #if defined(DMA_TIME_TEST) && DMA_TIME_TEST
    #if !(defined(DMA_TIME_TEST_READ) && DMA_TIME_TEST_READ)
    mstar_mci_PIU_Timer1_Start();
    #endif
    #endif

    if (pData_st->flags & MMC_DATA_WRITE)
    {
        sglen = dma_map_sg(mmc_dev(pHost_st->mmc), pData_st->sg, pData_st->sg_len, DMA_TO_DEVICE);

        for(i=0; i<sglen; i++)
        {
            pSG_st = &(pData_st->sg[i]);

            dmaaddr = sg_dma_address(pSG_st);
            dmalen = sg_dma_len(pSG_st);

            #ifdef CONFIG_MSTAR_AMBER3
            if( dmaaddr >= MSTAR_MIU1_BUS_BASE)
            {
                dmaaddr -= MSTAR_MIU1_BUS_BASE;
                fcie_setbits(pHost_st, FCIE_MIU_DMA1, MIU1_SELECT);
            }
            else
            {
                dmaaddr -= MSTAR_MIU0_BUS_BASE;
                fcie_clrbits(pHost_st, FCIE_MIU_DMA1, MIU1_SELECT);
            }
            #endif

            fcie_reg_write(pHost_st, FCIE_JOB_BL_CNT, (dmalen / 512));

            fcie_reg_write(pHost_st, SDIO_MIU_DMA0, (dmaaddr & 0xFFFF));
            fcie_reg_write(pHost_st, SDIO_MIU_DMA1, (dmaaddr >> 16));

            fcie_reg_write(pHost_st, FCIE_MMA_PRI_REG, (fcie_reg_read(pHost_st, FCIE_MMA_PRI_REG) | JOB_RW_DIR));

            //wait clock sync
            udelay(1);

            wait_fifoclk_ready(pHost_st);

            #if defined(MSTAR_MCI_IRQ) && MSTAR_MCI_IRQ
            fcie_setbits(pHost_st, FCIE_MIE_INT_EN, CARD_DMA_END);
            #endif

            fcie_reg_write(pHost_st, FCIE_MIE_PATH_CTL, (fcie_reg_read(pHost_st, FCIE_MIE_PATH_CTL) | MMA_ENABLE));
            fcie_reg_write(pHost_st, FCIE_SD_CTL, (fcie_reg_read(pHost_st, FCIE_SD_CTL) | (SD_DTRX_DIR | SD_DTRX_EN)));

            if( mstar_mci_wait_event(pHost_st, MMA_DATA_END|SD_DATA_END|CARD_DMA_END|MMA_LAST_DONE, LOOP_DELAY_WAIT_COMPLETE) == LOOP_DELAY_WAIT_COMPLETE )
            {
                printk("H: Wait MMA last done timeout!\n");
                while(1);
            }

            pData_st->bytes_xfered += pSG_st->length;
        }

        dma_unmap_sg(mmc_dev(pHost_st->mmc), pData_st->sg, pData_st->sg_len, DMA_TO_DEVICE);

        #if defined(DMA_TIME_TEST) && DMA_TIME_TEST
        #if !(defined(DMA_TIME_TEST_READ) && DMA_TIME_TEST_READ)
        u32Ticks = mstar_mci_PIU_Timer1_End();

        totalwritelen += pData_st->bytes_xfered;
        totalwritetime += u32Ticks;

        printk("WL: %08X\n", totalwritelen);
        printk("WT: %08X\n", totalwritetime);
        #endif
        #endif

        return 1;

    }

    return 0;
}

static void mstar_mci_switch_sd(struct mstar_mci_host *pHost_st)
{
    /* 1. Switch interface to SD */
    //fcie_reg_write(pHost_st, FCIE_MIE_PATH_CTL, SD_EN);
    /* 2. Set SD clock */
    //fcie_writew(pHost_st->sd_clk, REG_CLK_NFIE);
    /* 3. Set SD mode */
    fcie_reg_write(pHost_st, FCIE_SD_MODE, pHost_st->sd_mod);
    /* 3. Disable SDIO register */
    fcie_reg_write(pHost_st, SDIO_MIU_DMA0, 0);
    fcie_reg_write(pHost_st, SDIO_MIU_DMA1, 0);

    // Clear MIE event (write 1 clear)
    fcie_reg_write(pHost_st, FCIE_MIE_EVENT, SD_ALL_INT);

}

static void mstar_mci_enable(struct mstar_mci_host *pHost_st)
{
    /* Parameter out-of-bound check */
    if (!pHost_st)
    {
        printk(KERN_WARNING "mstar_mci_enable parameter pHost_st is NULL\n");
        return;
    }
    /* End Parameter out-of-bound check */

    /* init registers */
    fcie_reg_write(pHost_st, FCIE_MIE_INT_EN, 0);
    fcie_reg_write(pHost_st, FCIE_SD_CTL, 0);
    fcie_reg_write(pHost_st, FCIE_SD_MODE, 0);

    /* clear events */
    fcie_reg_write(pHost_st, FCIE_SD_STS, SD_STS_ERRORS);

    #ifdef CONFIG_MSTAR_AMBER3

    #if defined(BYPASS_MODE_ENABLE) && BYPASS_MODE_ENABLE

    fcie_writew(0x0040, REG_EMMC_CONFIG);
    fcie_reg_write(pHost_st, EMMC_BOOT_CONFIG, 0x0400);

    #else

    fcie_writew(0x0080, REG_EMMC_CONFIG);
    fcie_reg_write(pHost_st, EMMC_BOOT_CONFIG, 0x0200);

    #endif

    // Set MIU select determined by FCIE, not MIU itself
    fcie_writew(fcie_readw(0x1f200de8)|BIT9, 0x1f200de8);

    #endif
}

static void mstar_mci_disable(struct mstar_mci_host *pHost_st)
{
    /* Parameter out-of-bound check */
    if (!pHost_st)
    {
        printk(KERN_WARNING "mstar_mci_disable parameter pHost_st is NULL\n");
        return;
    }
    /* End Parameter out-of-bound check */

    fcie_reg_write(pHost_st, FCIE_MIE_PATH_CTL, ~(MMA_ENABLE | SD_EN));
}

static void mstar_mci_send_command(struct mstar_mci_host *pHost_st,
                                      struct mmc_command *pCmd_st)
{
    /* Define Local Variables */
    #if defined(CMD_ARG_DEBUG) && CMD_ARG_DEBUG
    int i;
    u8 pTemp[6];
    #endif
    struct mmc_data *pData_st;
    u32 mie_int = 0, sd_ctl = 0;

    #if defined(DMA_TIME_TEST) && DMA_TIME_TEST
    #if (defined(DMA_TIME_TEST_READ) && DMA_TIME_TEST_READ)
    u32 u32Ticks = 0;
    #endif
    #endif

    #if defined(TOG_TIME_TEST)&&TOG_TIME_TEST
    u32 u32Ticks = 0;
    #endif

    /* Parameter out-of-bound check */
    if (!pHost_st)
    {
        printk(KERN_WARNING "mstar_mci_send_command parameter pHost_st is NULL\n");
        return;
    }

    if (!pCmd_st)
    {
        printk(KERN_WARNING "mstar_mci_send_command parameter pCmd_st is NULL\n");
        return;
    }
    /* End Parameter out-of-bound check */

    pr_debug("mstar_mci_send_command\n");
    mstar_mci_switch_sd(pHost_st);

    pData_st = pCmd_st->data;

    pHost_st->cmd = pCmd_st;
    fcie_reg_write(pHost_st, FCIE_SD_CTL, sd_ctl);

    if (pData_st)
    {
        pr_debug("pData_st!=0\n");

        if (pData_st->blksz & 0x3)
        {
            pr_debug("Unsupported block size\n");
            pCmd_st->error = -EINVAL;
            mmc_request_done(pHost_st->mmc, pHost_st->request);
            return;
        }

        if (pData_st->flags & MMC_DATA_READ)
        {
            sd_ctl |= SD_DTRX_EN;

            // Enable stoping read clock when using scatter list DMA
            if( (pData_st->sg_len > 1) && (pCmd_st->opcode == 18) )
                fcie_setbits(pHost_st, FCIE_SD_MODE, SD_DMA_RD_CLK_STOP);
            else
                fcie_clrbits(pHost_st, FCIE_SD_MODE, SD_DMA_RD_CLK_STOP);
        }
    }

    sd_ctl |= SD_CMD_EN;
    mie_int |= SD_CMD_ENDE;

    if (pData_st)
    {
        //wait_fifoclk_ready(pHost_st);

        fcie_reg_write(pHost_st, FCIE_SD_MODE, fcie_reg_read(pHost_st, FCIE_SD_MODE) & ~SD_DEST);

        pData_st->bytes_xfered = 0;

        fcie_reg_write(pHost_st, SDIO_BLK_SIZE, (SDIO_BLK_MOD_EN|(u16)pData_st->blksz));
        pr_debug("[SDIO_BLK_SIZE]:%X\n", fcie_reg_read(pHost_st, SDIO_BLK_SIZE));

        if (pData_st->flags & MMC_DATA_READ)
        {
            /* Handle a read */
            #if defined(MSTAR_MCI_IRQ) && MSTAR_MCI_IRQ
            // We should wait for the MMA_LAST_DONE for read to make sure all data has been in DRAM
            if( (pCmd_st->opcode != 17) && (pCmd_st->opcode != 18) )
                mie_int |= MMA_LAST_DONE;
            #endif

            mstar_mci_pre_dma_read(pHost_st);

            fcie_reg_write(pHost_st, FCIE_MMA_PRI_REG, (fcie_reg_read(pHost_st, FCIE_MMA_PRI_REG) & (~JOB_RW_DIR)));

            //wait clock sync
            udelay(1);

            wait_fifoclk_ready(pHost_st);

            fcie_reg_write(pHost_st, FCIE_MIE_PATH_CTL, (fcie_reg_read(pHost_st, FCIE_MIE_PATH_CTL) | MMA_ENABLE));
        }
    }

    pr_debug("FCIE_SD_STS = %08X\n",fcie_reg_read(pHost_st, FCIE_SD_STS));

    #if 0
    if (waitD0High(pHost_st, LOOP_DELAY_WAIT_COMPLETE))
    {
        printk(KERN_WARNING "Card busy!!\n");
        pCmd_st->error = -ETIMEDOUT;
        return;
    }
    #endif

    /* Send the command and then enable the PDC - not the other way round as */
    /* the data sheet says */
    fcie_writew((((pCmd_st->arg >> 24) << 8) | (0x40 | pCmd_st->opcode)), FCIE1_BASE);
    fcie_writew(((pCmd_st->arg & 0xFF00) | ((pCmd_st->arg >> 16) & 0xFF)), FCIE1_BASE + 4);
    fcie_writew(((fcie_readw(FCIE1_BASE + 8) & 0xFF00) + (pCmd_st->arg & 0xFF)), FCIE1_BASE + 8);

    #if defined(CMD_ARG_DEBUG) && CMD_ARG_DEBUG
    printk("CMD: ");
    for (i=0; i < 6; i++)
    {
        pTemp[(3 - (i % 4)) + (4 * (i / 4))] =
            (u8)(fcie_readw(FCIE1_BASE + (((i + 1) / 2) * 4)) >> (8 * ((i + 1) % 2)));
        printk("%02X ", pTemp[i]);
    }
    printk("\n");
    #endif

    fcie_reg_write(pHost_st, FCIE_CMD_SIZE, 5);

    if (mmc_resp_type(pCmd_st) == MMC_RSP_NONE)
    {
        sd_ctl &= ~SD_RSP_EN;
        fcie_reg_write(pHost_st, FCIE_RSP_SIZE, 0);
    }
    else
    {
        if (mmc_resp_type(pCmd_st) == MMC_RSP_R2)
        {
            sd_ctl |= SD_RSPR2_EN;
            fcie_reg_write(pHost_st, FCIE_RSP_SIZE, 16); /* (136-8)/8 */
        }
        else
        {
            fcie_reg_write(pHost_st, FCIE_RSP_SIZE, 5); /*(48-8)/8 */
        }

        sd_ctl |= SD_RSP_EN;
    }

    #if (defined(TOG_TIME_TEST) && TOG_TIME_TEST)
    mstar_mci_PIU_Timer1_Start();
    #endif

    #ifdef CONFIG_MSTAR_AMBER3
    if( pHost_st->mmc->caps & MMC_CAP_1_8V_DDR )
    {
        if( (pCmd_st->opcode == 17) || (pCmd_st->opcode == 18) )
        {
            mstar_mci_set_toggle_count(pHost_st);
            mstar_mci_reset_toggle_count(pHost_st);
        }
        else if( (pCmd_st->opcode == 24) || (pCmd_st->opcode == 25) )
        {
            mstar_mci_set_toggle_count(pHost_st);
            mstar_mci_reset_toggle_count(pHost_st);
            mstar_mci_disable_stop_clock(pHost_st);
        }
        else if( pCmd_st->opcode == 12 )
            mstar_mci_disable_stop_clock(pHost_st);
    }
    #endif

    #if (defined(TOG_TIME_TEST) && TOG_TIME_TEST)
    u32Ticks = mstar_mci_PIU_Timer1_End();
    totalregtime += u32Ticks;
    #endif

    simple_debug("Sending command %d as %08X, mod %08X, arg = %08X\n",
             pCmd_st->opcode, sd_ctl, fcie_reg_read(pHost_st, FCIE_SD_MODE), pCmd_st->arg);
    simple_debug("Clock:0x%X\n", fcie_readw(REG_CLK_NFIE));

    #if defined(MSTAR_MCI_IRQ) && MSTAR_MCI_IRQ
	fcie_reg_write(pHost_st, FCIE_MIE_INT_EN, mie_int);
	pr_debug("[FCIE_MIE_INT_EN]:%X\n", fcie_reg_read(pHost_st, FCIE_MIE_INT_EN));
	pr_debug("[FCIE_MIE_EVENT]:%X\n", fcie_reg_read(pHost_st, FCIE_MIE_EVENT));
    #endif

    #if defined(DMA_TIME_TEST) && DMA_TIME_TEST
    #if defined(DMA_TIME_TEST_READ) && DMA_TIME_TEST_READ
    if( pCmd_st->opcode== 18 )
        mstar_mci_PIU_Timer1_Start();
    #endif
    #endif

	fcie_reg_write(pHost_st, FCIE_SD_CTL, sd_ctl);

    if( mstar_mci_wait_event(pHost_st, SD_CMD_END, LOOP_DELAY_WAIT_COMPLETE) == LOOP_DELAY_WAIT_COMPLETE )
    {
        printk("Wait CMD END timeout!\n");
        fcie_reg_dump(pHost_st);
        while(1);
    }
    else
    {
        if( mstar_mci_handle_cmdrdy(pHost_st) )
        {
            pr_debug("Completed command\n");

            mstar_mci_completed_command(pHost_st);

            return;
        }
        else
        {
            pr_debug("\033[34mIrq no complete!\033[0m\n");

            if( pData_st->flags & MMC_DATA_READ )
            {
                #if defined(MSTAR_MCI_IRQ) && MSTAR_MCI_IRQ
                // We should wait for the MMA_LAST_DONE for read to make sure all data has been in DRAM
                if( (pCmd_st->opcode == 17) || (pCmd_st->opcode == 18) )
                    fcie_setbits(pHost_st, FCIE_MIE_INT_EN, MMA_LAST_DONE);
                #endif

                if( mstar_mci_wait_event(pHost_st, MMA_DATA_END|SD_DATA_END|CARD_DMA_END|MMA_LAST_DONE, LOOP_DELAY_WAIT_COMPLETE) == LOOP_DELAY_WAIT_COMPLETE )
                {
                    printk("S: Wait MMA last done timeout!\n");
                    while(1);
                }
                else
                {
                    mstar_mci_post_dma_read(pHost_st);

                    mstar_mci_completed_command(pHost_st);

                    #if defined(DMA_TIME_TEST) && DMA_TIME_TEST
                    #if defined(DMA_TIME_TEST_READ) && DMA_TIME_TEST_READ
                    u32Ticks = mstar_mci_PIU_Timer1_End();

                    totalreadlen += pData_st->bytes_xfered;
                    totalreadtime += u32Ticks;

                    printk("RL: %08X\n", totalreadlen);
                    printk("RT: %08X\n", totalreadtime);
                    #endif
                    #endif
                }
            }
        }
    }

}

static void mstar_mci_process_next(struct mstar_mci_host *pHost_st)
{
    #if defined(TOG_TIME_TEST) && TOG_TIME_TEST
    u32 u32Ticks = 0;
    #endif

    /* Parameter out-of-bound check */
    if (!pHost_st)
    {
        printk(KERN_WARNING "mstar_mci_process_next parameter pHost_st is NULL\n");
        return;
    }
    /* End Parameter out-of-bound check */

    if (!(pHost_st->flags & FL_SENT_COMMAND))
    {
        pr_debug("CMD%d\n", pHost_st->request->cmd->opcode);
        pHost_st->flags |= FL_SENT_COMMAND;

        #if defined(CMD_TIME_TEST) && CMD_TIME_TEST
        #if !(defined(TOG_TIME_TEST) && TOG_TIME_TEST)
        if( (pHost_st->request->cmd->data) && (pHost_st->request->cmd->data->flags & MMC_DATA_READ) )
        {
            mstar_mci_PIU_Timer1_Start();
        }
        #endif //TOG_TIME_TEST
        #endif //CMD_TIME_TEST

        mstar_mci_send_command(pHost_st, pHost_st->request->cmd);

        #if defined(CMD_TIME_TEST) && CMD_TIME_TEST
        #if !(defined(TOG_TIME_TEST) && TOG_TIME_TEST)
        if( (pHost_st->request->cmd->data) && (pHost_st->request->cmd->data->flags & MMC_DATA_READ) )
        {
            u32Ticks = mstar_mci_PIU_Timer1_End();

            totalcmdlen += pHost_st->request->cmd->data->bytes_xfered;
            totalcmdtime += u32Ticks;

            printk("CMDL: %08X\n", totalcmdlen);
            printk("CMDT: %08X\n", totalcmdtime);
        }
        #endif //TOG_TIME_TEST

        #if (defined(TOG_TIME_TEST) && TOG_TIME_TEST)
        if( (pHost_st->request->cmd->data) && (pHost_st->request->cmd->data->flags & MMC_DATA_READ) )
        {
            totalcmdlen += pHost_st->request->cmd->data->bytes_xfered;

            printk("REGL: %08X\n", totalcmdlen);
            printk("REGT: %08X\n", totalregtime);
        }
        #endif

        #endif //CMD_TIME_TEST
    }
    else if ((!(pHost_st->flags & FL_SENT_STOP)) && pHost_st->request->stop)
    {
        pr_debug("CMD%d\n", pHost_st->request->stop->opcode);
        pHost_st->flags |= FL_SENT_STOP;
        mstar_mci_send_command(pHost_st, pHost_st->request->stop);
    }
    else
    {
        pr_debug("mmc_request_done\n");
        mmc_request_done(pHost_st->mmc, pHost_st->request);
    }

	//waitD0High(pHost_st, LOOP_DELAY_WAIT_COMPLETE);
}

static void mstar_mci_completed_command(struct mstar_mci_host *pHost_st)
{
    /* Define Local Variables */
    u8 *pTemp = 0;
    u32 status = 0, i = 0;
    struct mmc_command *pCmd_st;
    struct mmc_data *pData_st;

    /* Parameter out-of-bound check */
    if (!pHost_st)
    {
        printk(KERN_WARNING "mstar_mci_completed_command parameter pHost_st is NULL\n");
        return;
    }

	pCmd_st = pHost_st->cmd;
	pData_st = pCmd_st->data;

    /* End Parameter out-of-bound check */

    if( (pCmd_st->flags & MMC_RSP_R1B) || (pData_st && (pData_st->flags & MMC_DATA_WRITE)) )
    {
        waitD0High(pHost_st, LOOP_DELAY_WAIT_COMPLETE);
    }

    pTemp = (u8*)&(pCmd_st->resp[0]);

    #if defined(CMD_RSP_DEBUG) && CMD_RSP_DEBUG
    printk("RSP: ");
    #endif
    for (i=0; i < 15; i++)
    {
        pTemp[(3 - (i % 4)) + (4 * (i / 4))] =
            (u8)(fcie_readw(FCIE1_BASE + (((i + 1) / 2) * 4)) >> (8 * ((i + 1) % 2)));
        #if defined(CMD_RSP_DEBUG) && CMD_RSP_DEBUG
        printk("%02X ", pTemp[i]);
        #endif
    }
    #if defined(CMD_RSP_DEBUG) && CMD_RSP_DEBUG
    printk("\n");
    #endif

    status = fcie_reg_read(pHost_st, FCIE_SD_STS);

    simple_debug( "Status = %08X [%08X %08X %08X %08X]\n",
              status, pCmd_st->resp[0], pCmd_st->resp[1], pCmd_st->resp[2], pCmd_st->resp[3]);

    if (status & SD_STS_ERRORS)
    {
        if ((status & SD_CMDRSP_CERR) && !(mmc_resp_type(pCmd_st) & MMC_RSP_CRC))
        {
            pCmd_st->error = 0;
        }
        else
        {
            //printk(KERN_WARNING "r = %x\n", status);
            if (status & (SD_DAT_NORSP | SD_DAT_STSERR))
            {
                printk("SD_STS=%04X\n", status);
                pCmd_st->error = -ETIMEDOUT;
            }
            else if (status & (SD_CMDRSP_CERR | SD_DAT_CERR | SD_DAT_STSNEG))
            {
                printk("SD_STS=%04X\n", status);
                pCmd_st->error = -EILSEQ;
            }
            else
            {
                printk("SD_STS=%04X\n", status);
                pCmd_st->error = -EIO;
            }

            printk(KERN_ERR "Error detected and set to %d (cmd = %d, retries = %d)\n",
                     pCmd_st->error, pCmd_st->opcode, pCmd_st->retries);
            fcie_reg_dump(pHost_st);
        }
    }
    else
    {
        pCmd_st->error = 0;
    }

    clear_fcie_status(pHost_st, FCIE_SD_STS, SD_STS_ERRORS);

    mstar_mci_process_next(pHost_st);
}

static void mstar_mci_request(struct mmc_host *pMMC_st, struct mmc_request *pMRQ_st)
{
    /* Define Local Variables */
    struct mstar_mci_host *pHost_st = mmc_priv(pMMC_st);

    #if defined(REQ_TIME_TEST) && REQ_TIME_TEST
    u32 u32Ticks = 0;
    #endif

    /* Parameter out-of-bound check */
    if (!pMMC_st)
    {
        printk(KERN_WARNING "mstar_mci_request parameter pMMC_st is NULL\n");
        return;
    }

    if (!pMRQ_st)
    {
        printk(KERN_WARNING "mstar_mci_request parameter pMRQ_st is NULL\n");
        return;
    }
    /* End Parameter out-of-bound check */

    pHost_st->request = pMRQ_st;
    pHost_st->flags = 0;

    #if defined(REQ_TIME_TEST) && REQ_TIME_TEST
    if( (pMRQ_st->cmd->data) && (pMRQ_st->cmd->data->flags & MMC_DATA_READ) )
    {
        mstar_mci_PIU_Timer1_Start();
    }
    #endif

    mstar_mci_process_next(pHost_st);

    #if defined(REQ_TIME_TEST) && REQ_TIME_TEST
    if( (pMRQ_st->cmd->data) && (pMRQ_st->cmd->data->flags & MMC_DATA_READ) )
    {
        u32Ticks = mstar_mci_PIU_Timer1_End();

        totalreqlen += pMRQ_st->cmd->data->bytes_xfered;
        totalreqtime += u32Ticks;

        printk("REQL: %08X\n", totalreqlen);
        printk("REQT: %08X\n", totalreqtime);
    }
    #endif

}

static void mstar_mci_set_ios(struct mmc_host *pMMC_st, struct mmc_ios *pIOS_st)
{
    /* Define Local Variables */
    s32 clkdiv = 0;
    u32 sd_mod = 0;
    u16 reg_mask = ~((u16)0x3F);

    struct mstar_mci_host *pHost_st = mmc_priv(pMMC_st);

    /* Parameter out-of-bound check */
    if (!pMMC_st)
    {
        printk(KERN_WARNING "mstar_mci_set_ios parameter pMMC_st is NULL\n");
        return;
    }

    if (!pIOS_st)
    {
        printk(KERN_WARNING "mstar_mci_set_ios parameter pIOS_st is NULL\n");
        return;
    }
    /* End Parameter out-of-bound check */

    pr_debug("MMC: mstar_mci_set_ios\n");

    sd_mod = 0;
    clkdiv =fcie_readw(REG_CLK_NFIE); /* High triggle level */

    pr_debug("clkdiv:%d\n", clkdiv);
    pr_debug("clock:%d\n", pIOS_st->clock);
    if (pIOS_st->clock == 0)
    {
        /* Disable the MCI controller */
        sd_mod &= ~(SD_CLK_EN | SD_CS_EN);
        clkdiv |= 0x01; /* disable clock */
    }
    else
    {
        /* Enable the MCI controller */
        #if (defined(SDR_MODE_ENABLE) && SDR_MODE_ENABLE) || (defined(DDR_MODE_ENABLE) && DDR_MODE_ENABLE)
        sd_mod |= (SD_CLK_EN | SD_CS_EN);
        #else
        sd_mod |= (SD_CLK_EN | SD_CS_EN | SD_DATSYNC);
        #endif

        clkdiv &= reg_mask;

        #if defined(CLK_48M) && CLK_48M
        if( pIOS_st->clock >= CLK_48MHz )
        {
            printk("FCIE3 set 48MHz...\n");
            clkdiv |= (BIT_FCIE_CLK_48M);
            #ifdef CONFIG_MSTAR_AMBER3
            if( pHost_st->mmc->caps & MMC_CAP_1_8V_DDR )
                clkdiv |= (BIT_EMMC_CLK_192M);
            #endif
        }
        else
        #endif // CLK_48M

        #if defined(CLK_43M) && CLK_43M
        if( pIOS_st->clock >= CLK_43MHz )
        {
            printk("FCIE3 set 43MHz...\n");
            clkdiv |= (BIT_FCIE_CLK_43M);
        }
        else
        #endif // CLK_43M

        #if defined(CLK_40M) && CLK_40M
        if( pIOS_st->clock >= CLK_40MHz )
		{
			printk("FCIE3 set 40MHz...\n");
			clkdiv |= (BIT_FCIE_CLK_40M);
            #ifdef CONFIG_MSTAR_AMBER3
            if( pHost_st->mmc->caps & MMC_CAP_1_8V_DDR )
                clkdiv |= (BIT_EMMC_CLK_160M);
            #endif
        }
        else
        #endif

        #if defined(CLK_36M) && CLK_36M
        if( pIOS_st->clock >= CLK_36MHz )
		{
			printk("FCIE3 set 36MHz...\n");
			clkdiv |= (BIT_FCIE_CLK_36M);
            #ifdef CONFIG_MSTAR_AMBER3
            if( pHost_st->mmc->caps & MMC_CAP_1_8V_DDR )
                clkdiv |= (BIT_EMMC_CLK_144M);
            #endif
        }
        else
        #endif

        #if defined(CLK_32M) && CLK_32M
        if( pIOS_st->clock >= CLK_32MHz )
        {
            printk("FCIE3 set 32MHz...\n");
            clkdiv |= (BIT_FCIE_CLK_32M);
        }
        else
        #endif // CLK_32M

        #if defined(CLK_27M) && CLK_27M
        if( pIOS_st->clock >= CLK_27MHz )
        {
			printk("FCIE3 set 27MHz...\n");
			clkdiv |= (BIT_FCIE_CLK_27M);
            #ifdef CONFIG_MSTAR_AMBER3
            if( pHost_st->mmc->caps & MMC_CAP_1_8V_DDR )
                clkdiv |= (BIT_EMMC_CLK_108M);
            #endif
		}
        else
        #endif

        #if defined(CLK_20M) && CLK_20M
        if( pIOS_st->clock >= CLK_20MHz )
        {
			printk("FCIE3 set 20MHz...\n");
			clkdiv |= (BIT_FCIE_CLK_20M);
            #ifdef CONFIG_MSTAR_AMBER3
            if( pHost_st->mmc->caps & MMC_CAP_1_8V_DDR )
                clkdiv |= (BIT_EMMC_CLK_80M);
            #endif
        }
        else
        #endif
		{
			printk("FCIE3 set 300KHz...\n");
            clkdiv |= (BIT_FCIE_CLK_300K);
        }

        fcie_writew(clkdiv, REG_CLK_NFIE);

        pHost_st->sd_clk = (u16)clkdiv;
        pr_debug("[clkdiv]=%d\n", clkdiv);
        pr_debug("[pHost_st->sd_clk]=%d\n", pHost_st->sd_clk);

    }

	if (pIOS_st->bus_width == MMC_BUS_WIDTH_8)
    {
        pr_debug("\033[31mMMC: Setting controller bus width to 8\033[m\n");
        sd_mod = (sd_mod & ~SD_DAT_LINE) | SD_DAT_LINE1;
    }
    else if (pIOS_st->bus_width == MMC_BUS_WIDTH_4)
    {
        pr_debug("\033[31mMMC: Setting controller bus width to 4\033[m\n");
        sd_mod = (sd_mod & ~SD_DAT_LINE) | SD_DAT_LINE0;
    }
    else
    {
        pr_debug("MMC: Setting controller bus width to 1\n");
        sd_mod = (sd_mod & ~SD_DAT_LINE);
    }

    #ifdef CONFIG_MSTAR_AMBER3
    if( pMMC_st->ios.ddr != 0 )
        mstar_mci_config_ddr_mode(pHost_st);
    #endif

    pr_debug("[FCIE_SD_MODE]=%x\n", sd_mod);
    pHost_st->sd_mod = sd_mod;

}

#if defined(MSTAR_MCI_IRQ) && MSTAR_MCI_IRQ
static irqreturn_t mstar_mci_irq(s32 irq, void *pDevID)
{
    struct mstar_mci_host *pHost_st = pDevID;
    volatile u32 mie_evt = fcie_reg_read(pHost_st, FCIE_MIE_EVENT);

    if( mie_evt & SD_CMD_END )
    {
        fcie_clrbits(pHost_st, FCIE_MIE_INT_EN, SD_CMD_END);

        atomic_set(&fcie_int, 1);
        wake_up(&fcie_wait);
    }

    if( mie_evt & MMA_LAST_DONE )
    {
        fcie_clrbits(pHost_st, FCIE_MIE_INT_EN, MMA_LAST_DONE);

        atomic_set(&fcie_int, 1);
        wake_up(&fcie_wait);
    }

    if( mie_evt & CARD_DMA_END )
    {
        fcie_clrbits(pHost_st, FCIE_MIE_INT_EN, CARD_DMA_END);

        atomic_set(&fcie_int, 1);
        wake_up(&fcie_wait);
    }

    return IRQ_HANDLED;
}
#endif

static s32 mstar_mci_get_ro(struct mmc_host *pMMC_st)
{
    /* Parameter out-of-bound check */
    if (!pMMC_st)
    {
        printk(KERN_WARNING "mstar_mci_get_ro parameter pMMC_st is NULL\n");
        return -EINVAL;
    }
    /* End Parameter out-of-bound check */

    return 0;
}

static s32 mstar_mci_probe(struct platform_device *pDev_st)
{
    /* Define Local Variables */
    struct mmc_host *pMMC_st        = 0;
    struct mstar_mci_host *pHost_st = 0;
    struct resource *pRes_st        = 0;

    #if defined(MSTAR_MCI_IRQ) && MSTAR_MCI_IRQ
    s32 ret = 0;
    #endif

    printk("Probe MCI devices\n");
    /* Parameter out-of-bound check */
    if (!pDev_st)
    {
        printk(KERN_WARNING "mstar_mci_probe parameter pDev_st is NULL\n");
        return -EINVAL;
    }
    /* End Parameter out-of-bound check */

    pRes_st = platform_get_resource(pDev_st, IORESOURCE_MEM, 0);
    if (!pRes_st)
    {
        return -ENXIO;
    }

    if (!request_mem_region(pRes_st->start, pRes_st->end - pRes_st->start + 1, DRIVER_NAME))
    {
        return -EBUSY;
    }

    pMMC_st = mmc_alloc_host(sizeof(struct mstar_mci_host), &pDev_st->dev);
    if (!pMMC_st)
    {
        pr_debug("Failed to allocate mmc host\n");
        release_mem_region(pRes_st->start, pRes_st->end - pRes_st->start + 1);
        return -ENOMEM;
    }

    pMMC_st->ops = &sg_mstar_mci_ops;
    pMMC_st->f_min = CLK_300KHz;
	pMMC_st->f_max = CLK_48MHz;

    pMMC_st->ocr_avail = MMC_VDD_27_28 | MMC_VDD_28_29 | MMC_VDD_29_30 | MMC_VDD_30_31 | \
						 MMC_VDD_31_32 | MMC_VDD_32_33 | MMC_VDD_33_34 | MMC_VDD_165_195;

    pMMC_st->max_blk_count  = 4095;
    pMMC_st->max_blk_size   = 512; /* sector */
    pMMC_st->max_req_size   = pMMC_st->max_blk_count  * pMMC_st->max_blk_size;
    pMMC_st->max_seg_size   = pMMC_st->max_req_size;

    pMMC_st->max_phys_segs  = 128;
    pMMC_st->max_hw_segs    = 128;

    pHost_st           = mmc_priv(pMMC_st);
    pHost_st->mmc      = pMMC_st;

    pMMC_st->caps = MMC_CAP_8_BIT_DATA | MMC_CAP_MMC_HIGHSPEED | MMC_CAP_NONREMOVABLE;

    #ifdef CONFIG_MSTAR_AMBER3
    #if defined(DDR_MODE_ENABLE) && DDR_MODE_ENABLE
    pMMC_st->caps |= MMC_CAP_1_8V_DDR;
    #endif
    #endif

    pHost_st->baseaddr = (void __iomem *)FCIE0_BASE;

    mmc_add_host(pMMC_st);

    platform_set_drvdata(pDev_st, pMMC_st);

    mstar_mci_enable(pHost_st);

    #if defined(MSTAR_MCI_IRQ) && MSTAR_MCI_IRQ
    /***** Allocate the MCI interrupt *****/
    pHost_st->irq = platform_get_irq(pDev_st, 0);
    ret = request_irq(pHost_st->irq, mstar_mci_irq, IRQF_SHARED, DRIVER_NAME, pHost_st);
    if (ret)
    {
        printk(KERN_ERR "Mstar MMC: Failed to request MCI interrupt\n");
        mmc_free_host(pMMC_st);
        release_mem_region(pRes_st->start, pRes_st->end - pRes_st->start + 1);
        return ret;
    }

    pr_debug("Mstar MMC: request MCI interrupt = %d\n", pHost_st->irq);
    #endif

    return 0;
}

static s32 __exit mstar_mci_remove(struct platform_device *pDev_st)
{
    /* Define Local Variables */
    struct mmc_host *pMMC_st        = platform_get_drvdata(pDev_st);
    struct mstar_mci_host *pHost_st = mmc_priv(pMMC_st);
    struct resource *pRes_st        = 0;

    /* Parameter out-of-bound check */
    if (!pDev_st)
    {
        printk(KERN_WARNING "mstar_mci_remove parameter pDev_st is NULL\n");
        return -EINVAL;
    }
    /* End Parameter out-of-bound check */

    if (!pMMC_st)
    {
        return -1;
    }

    mstar_mci_disable(pHost_st);
    mmc_remove_host(pMMC_st);

    #if defined(MSTAR_MCI_IRQ) && MSTAR_MCI_IRQ
    free_irq(pHost_st->irq, pHost_st);
    #endif

    pRes_st = platform_get_resource(pDev_st, IORESOURCE_MEM, 0);
    release_mem_region(pRes_st->start, pRes_st->end - pRes_st->start + 1);

    mmc_free_host(pMMC_st);
    platform_set_drvdata(pDev_st, NULL);
    pr_debug("MCI Removed\n");

    return 0;
}


#ifdef CONFIG_PM
static s32 mstar_mci_suspend(struct platform_device *pDev_st, pm_message_t state)
{
    /* Define Local Variables */
    struct mmc_host *pMMC_st = platform_get_drvdata(pDev_st);
    s32 ret = 0;

    if (pMMC_st)
    {
        ret = mmc_suspend_host(pMMC_st, state);
    }

    pr_debug("MCI suspend ret=%d\n", ret);
    return ret;
}

static s32 mstar_mci_resume(struct platform_device *pDev_st)
{
    /* Define Local Variables */
    struct mmc_host *pMMC_st = platform_get_drvdata(pDev_st);
    s32 ret = 0;

    if (pMMC_st)
    {
        ret = mmc_resume_host(pMMC_st);
    }

    pr_debug("MCI resume ret=%d\n", ret);
    return ret;
}
#endif  /* End ifdef CONFIG_PM */

/******************************************************************************
 * Define Static Global Variables
 ******************************************************************************/

/* MSTAR MCI Driver (Include Remove, Suspend, and Resume) */
static struct platform_driver sg_mstar_mci_driver =
{
	.probe = mstar_mci_probe,
    .remove = __exit_p(mstar_mci_remove),

    #ifdef CONFIG_PM
    .suspend = mstar_mci_suspend,
    .resume = mstar_mci_resume,
    #endif

    .driver  =
    {
        .name = DRIVER_NAME,
        .owner = THIS_MODULE,
    },
};

/******************************************************************************
 * Init & Exit Modules
 ******************************************************************************/
static s32 __init mstar_mci_init(void)
{
	int err = 0;

    printk(KERN_INFO "SD Card Init: %s.\n",__FUNCTION__);

    if( (err = platform_device_register(&sg_mstar_mmc_device_st)) < 0 )
		printk("\033[31mFail to register eMMC platform device\033[m\n");
    if( (err = platform_driver_register(&sg_mstar_mci_driver)) < 0 )
		printk("\033[31mFail to register eMMC platform driver\033[m\n");

	return err;
}

static void __exit mstar_mci_exit(void)
{
    platform_driver_unregister(&sg_mstar_mci_driver);
}

module_init(mstar_mci_init);
module_exit(mstar_mci_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Mstar Multimedia Card Interface driver");
MODULE_AUTHOR("Benson.Hsiao");
