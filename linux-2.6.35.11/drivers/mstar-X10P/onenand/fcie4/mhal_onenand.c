////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2010 MStar Semiconductor, Inc.
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

//-------------------------------------------------------------------------------------------------
//  Include Files
//-------------------------------------------------------------------------------------------------
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
// Added by Mstar, for semaphore  structure
#include <linux/semaphore.h>

#include "mdrv_types.h"
#include "mhal_onenand.h"
#include "mhal_onenand_reg.h"

#ifdef __cplusplus
extern "C"
{
#endif

//-------------------------------------------------------------------------------------------------
//  Driver Compiler Options
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Local Defines
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Global Variables
//-------------------------------------------------------------------------------------------------
// Added by MStar, declare Mutex for this driver and PCMCIA driver
#if defined(CONFIG_MSTAR_TITANIA9) || defined(CONFIG_MSTAR_AMBER1)
DECLARE_MUTEX(ond_mutex);

EXPORT_SYMBOL(ond_mutex);
#endif


//-------------------------------------------------------------------------------------------------
//  Local Variables
//-------------------------------------------------------------------------------------------------
static volatile U8 gu8Mode = RW_MODE_NONE;
wait_queue_head_t onif_wait;
int onif_int = 0;
// Added by Mstar, local variables declaration
U8 onif_direct_read = 0;
extern unsigned int Chip_Query_ID(void);

//-------------------------------------------------------------------------------------------------
//  Debug Functions
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Local Functions
//-------------------------------------------------------------------------------------------------
// Added by Mstar, functions of mutex operations
#if defined(CONFIG_MSTAR_TITANIA9)|| defined(CONFIG_MSTAR_AMBER1)
void nand_lock_fcie(void)
{
	down(&ond_mutex);
}
EXPORT_SYMBOL(nand_lock_fcie);

void nand_unlock_fcie(void)
{
	up(&ond_mutex);
}
EXPORT_SYMBOL(nand_unlock_fcie);
#endif

void HAL_ONIF_Switch_Pad(void)
{
	DBG_ONENAND(printk("%s\n", __FUNCTION__));
        // Removed by Mstar, useless code.
	//disable ph_nand_mode
	REG_CLR_BITS_UINT16(REG_NAND_MODE, BIT12|BIT11|BIT9|BIT8|BIT4);
	DBG_ONENAND(printf("REG_NAND_MODE: %04X\n", REG(REG_NAND_MODE)));

	//disable pcmconfig
	REG_CLR_BITS_UINT16(REG_PCMCONFIG, BIT1|BIT0);
	DBG_ONENAND(printf("REG_PCMCONFIG: %04X\n", REG(REG_PCMCONFIG)));
        // Removed by Mstar, useless code.
}

EXPORT_SYMBOL(HAL_ONIF_Switch_Pad);

void HAL_ONIF_Release_Pad(void)
{
	DBG_ONENAND(printk("%s\n", __FUNCTION__));
        // Removed by Mstar, useless code. X5 do not need theses configs.
}

EXPORT_SYMBOL(HAL_ONIF_Release_Pad);

#if defined(USE_POLLING) && USE_POLLING
U32 HAL_ONIF_Wait_Event(U16 u16Event)
{
	U32 u32TimeoutCount = 0;

	while( (REG(ONIF_EVENT) & u16Event) != u16Event )
	{
		if( (++u32TimeoutCount) > 0xFFFF )
		{
			(printk("Wait ONIF event %X timeout!!!\n", u16Event));
			return ONIF_WAIT_TIMEOUT;
		}
	}

	REG_WRITE_UINT16(ONIF_EVENT, u16Event);

	return ONIF_WAIT_OK;
}

EXPORT_SYMBOL(HAL_ONIF_Wait_Event);
#endif

#if !ASYNC_READ_RIU
static void HAL_ONIF_XIU_Read(unsigned short *pu16Buf, unsigned short u16StartAddr, int len)
{
	int i;

	DBG_ONENAND(printk("%s\n", __FUNCTION__));

	REG_WRITE_UINT16(ONIF_CONFIG0, R_DBF_INDEN | 0x0044);
	DBG_ONENAND(printk("ONIF_CONFIG0 = %04X\n", REG(ONIF_CONFIG0)));

	REG_WRITE_UINT16(ONIF_INDIR_ACCESS_START_ADDR, u16StartAddr);
	DBG_ONENAND(printk("ONIF_INDIR_ACCESS_START_ADDR = %04X\n", REG(ONIF_INDIR_ACCESS_START_ADDR)));

	for(i=0; i<len; i++)
		pu16Buf[i] = REG(ONIF_INDIR_ACCESS_START_DATA);

	REG_WRITE_UINT16(ONIF_CONFIG0, 0x0044);
	DBG_ONENAND(printk("ONIF_CONFIG0 = %04X\n", REG(ONIF_CONFIG0)));

	DBG_ONENAND(printk("%s finished\n", __FUNCTION__));
}
#endif

void HAL_ONIF_RW_Init(U8 u8Mode)
{
	if( gu8Mode == u8Mode )
		return;

	DBG_ONENAND(printk("%s(%s)\n", __FUNCTION__, (u8Mode == RW_MODE_SYNC) ? "sync" : "async"));

	REG_WRITE_UINT16(ONIF_CONFIG0, 0x0044);
	DBG_ONENAND(printk("ONIF_CONFIG0=%04X\n", REG(ONIF_CONFIG0)));

	#if defined(USE_POLLING) && USE_POLLING
	REG_WRITE_UINT16(ONIF_EVENT, 0);
	#else
	REG_WRITE_UINT16(ONIF_EVENT, R_CMDQ_EMPTY<<8);
	#endif
	DBG_ONENAND(printk("ONIF_EVENT=%04X\n", REG(ONIF_EVENT)));

	REG_WRITE_UINT16(ONIF_CMDQ_STATUS, R_CMDQ_EN);	// enable CMDQ
	DBG_ONENAND(printk("ONIF_CMDQ_STATUS=%04X\n", REG(ONIF_CMDQ_STATUS)));

	REG_WRITE_UINT16(ONIF_CMDQ_PARAM, REG_SYSTEM_CONFIGURATION_1);
	REG_WRITE_UINT16(ONIF_CMDQ_OPCODE, (0x40<<4)|OP_SEND_ADDR);

	if( u8Mode == RW_MODE_ASYNC )
		REG_WRITE_UINT16(ONIF_CMDQ_PARAM, 0x40E0);
	else if( u8Mode == RW_MODE_SYNC )
		REG_WRITE_UINT16(ONIF_CMDQ_PARAM, 0xC0E0);

	REG_WRITE_UINT16(ONIF_CMDQ_OPCODE, OP_ASYNC_WRITE);

	#if !(defined(USE_POLLING) && USE_POLLING)
	wait_event(onif_wait, (onif_int & R_CMDQ_EMPTY));
	REG_WRITE_UINT16(ONIF_EVENT, 0);
	onif_int = 0;
	#else
	if( HAL_ONIF_Wait_Event(R_CMDQ_EMPTY) )
	{
		printk("Set to async mode timeout\n");
	}
	#endif

	if( u8Mode == RW_MODE_ASYNC )
		REG_WRITE_UINT16(REG_CLK_ONIF, ONIF_CLK_SEL | ONIF_CLK_96M);
	else if( u8Mode == RW_MODE_SYNC )
		REG_WRITE_UINT16(REG_CLK_ONIF, ONIF_CLK_SEL | ONIF_CLK_54M);

	gu8Mode = u8Mode;

	DBG_ONENAND(printk("%s finished, ONIF_CONFIG0=%04X\n", __FUNCTION__, REG(ONIF_CONFIG0)));
}

//-------------------------------------------------------------------------------------------------
//  Global Functions
//-------------------------------------------------------------------------------------------------
extern void Chip_Flush_Memory(void);
extern void Chip_Read_Memory(void);

irqreturn_t HAL_ONIF_IRQ(int irq, void *dummy)
{
	U16 u16RegValue;

	u16RegValue = REG(ONIF_EVENT);
	REG_WRITE_UINT16(ONIF_EVENT, u16RegValue);
	
	onif_int |= (u16RegValue & 0xFF);
	
	wake_up(&onif_wait);	
	return IRQ_HANDLED;
}

void HAL_ONIF_INIT(void)
{
	DBG_ONENAND(printk("%s\n", __FUNCTION__));

	//bonding overwrite
	REG_SET_BITS_UINT16(CHIP_CONFIG_OVERWRITE, 0x0300);
	DBG_ONENAND(printk("CHIP_CONFIG_OVERWRITE(%08X)=%04X\n", CHIP_CONFIG_OVERWRITE, REG(CHIP_CONFIG_OVERWRITE)));

	DBG_ONENAND(printk("disable all pad in\n"));
	REG_CLR_BITS_UINT16(REG_ALLPAD_IN, BIT15);

        // Added by Mstar, if chip id is 0x2D, use direct read.
	if( Chip_Query_ID() == 0x2D )
		onif_direct_read = 1;

	REG_SET_BITS_UINT16(ONIF_MIU_PACTH, BIT0);

	REG_WRITE_UINT16(ONIF_ON_BOOTRAM_START_ADDR, BOOT_RAM_MAIN_0);
	DBG_ONENAND(printk("ONIF_ON_BOOTRAM_START_ADDR=%04X\n", REG(ONIF_ON_BOOTRAM_START_ADDR)));

	REG_WRITE_UINT16(ONIF_ON_DATARAM_START_ADDR, DATA_RAM_MAIN_0);
	DBG_ONENAND(printk("ONIF_ON_DATARAM_START_ADDR=%04X\n", REG(ONIF_ON_DATARAM_START_ADDR)));

	REG_WRITE_UINT16(ONIF_MIU_CTRL, R_PING_PONG_FIFO_CLK_EN);	// enable ping-pong fifo clk
	DBG_ONENAND(printk("ONIF_MIU_CTRL=%04X\n", REG(ONIF_MIU_CTRL)));

	REG_WRITE_UINT16(ONIF_DMA_PACKET_SIZE, 512);
	DBG_ONENAND(printk("ONIF_DMA_PACKET_SIZE=%04X\n", REG(ONIF_DMA_PACKET_SIZE)));

	#if !(defined(USE_POLLING) && USE_POLLING)
	init_waitqueue_head(&onif_wait);
	request_irq(10, HAL_ONIF_IRQ, SA_INTERRUPT, "ONIF", NULL);
	#endif

        // Removed by Mstar, useless code

	DBG_ONENAND(printk("%s finished\n", __FUNCTION__));
}

EXPORT_SYMBOL(HAL_ONIF_INIT);

void HAL_ONIF_Async_Write(U16 *pu16Buf, U16 u16Addr, U32 len)
{
	U32 i;

	DBG_ONENAND(printk("%s\n", __FUNCTION__));

	HAL_ONIF_Switch_Pad();

	HAL_ONIF_RW_Init(RW_MODE_ASYNC);

	REG_WRITE_UINT16(ONIF_CONFIG0, 0x0044);
	REG_WRITE_UINT16(ONIF_CMDQ_STATUS, R_CMDQ_EN);

	for(i=0; i<len; i++)
	{
		DBG_ONENAND(printk("Async write %dth byte\n", i));

		if( i == len - 1 )
			REG_WRITE_UINT16(ONIF_EVENT, R_CMDQ_EMPTY<<8); // test for irq crash in write

		REG_WRITE_UINT16(ONIF_CMDQ_PARAM, u16Addr+i);
		REG_WRITE_UINT16(ONIF_CMDQ_OPCODE, (0x40<<4)|OP_SEND_ADDR);

		REG_WRITE_UINT16(ONIF_CMDQ_PARAM, pu16Buf[i]);
		REG_WRITE_UINT16(ONIF_CMDQ_OPCODE, OP_ASYNC_WRITE);

		DBG_ONENAND(printk("ONIF_EVENT=%04X\n", REG(ONIF_EVENT)));
	}

	#if !(defined(USE_POLLING) && USE_POLLING) // test for irq crash in write
	wait_event(onif_wait, (onif_int & R_CMDQ_EMPTY));
	REG_WRITE_UINT16(ONIF_EVENT, 0);
	onif_int = 0;
	#else
	if( HAL_ONIF_Wait_Event(R_CMDQ_EMPTY) )
		(printk("%s: Wait CMDQ event timeout!\n", __FUNCTION__));
	#endif

	DBG_ONENAND(printk("%s finished, ONIF_CONFIG0=%04X\n", __FUNCTION__, REG(ONIF_CONFIG0)));

	HAL_ONIF_Release_Pad();
}

EXPORT_SYMBOL(HAL_ONIF_Async_Write);

#if !ASYNC_READ_RIU
void HAL_ONIF_Async_Read(U16 *pu16Buf, U16 u16Addr, U32 len)
{
	U32 i;

	DBG_ONENAND(printk("%s\n", __FUNCTION__));

	HAL_ONIF_RW_Init(RW_MODE_ASYNC);

	REG_WRITE_UINT16(ONIF_CONFIG0, R_ASYN_RDEST | R_SYNC_DBF_DEST | 0x0044);
	DBG_ONENAND(printk("ONIF_CONFIG0=%04X\n", REG(ONIF_CONFIG0)));

	REG_WRITE_UINT16(ONIF_CMDQ_PARAM, 0x0003);
	REG_WRITE_UINT16(ONIF_CMDQ_OPCODE, OP_WAIT_STATUS);

	for(i=0; i<len; i++)
	{
		DBG_ONENAND(printk("Async read %dth byte\n", i));

		REG_WRITE_UINT16(ONIF_CMDQ_PARAM, u16Addr+i);
		REG_WRITE_UINT16(ONIF_CMDQ_OPCODE, OP_SEND_ADDR);
		REG_WRITE_UINT16(ONIF_CMDQ_OPCODE, OP_ASYNC_READ);
	}

	#if defined(USE_POLLING) && USE_POLLING
	if( HAL_ONIF_Wait_Event(R_CMDQ_EMPTY) )
	{
		(printk("%s: Wait CMDQ event timeout!\n", __FUNCTION__));
	}
	#else
	wait_event(onif_wait, (onif_int & R_CMDQ_EMPTY));
	REG_WRITE_UINT16(ONIF_EVENT, 0);
	onif_int = 0;
	#endif

	REG_WRITE_UINT16(ONIF_CONFIG0, 0x0044);
	DBG_ONENAND(printk("ONIF_CONFIG0=%04X\n", REG(ONIF_CONFIG0)));

	HAL_ONIF_XIU_Read(pu16Buf, 0, len);

	DBG_ONENAND(printk("%s finished\n", __FUNCTION__));
}

EXPORT_SYMBOL(HAL_ONIF_Async_Read);
#endif

void HAL_ONIF_Async_Read2Reg(U16 *pu16Buf, U16 u16Addr, U32 len)
{
	U32 i;

	DBG_ONENAND(printk("%s(%p, %X, %X)\n", __FUNCTION__, pu16Buf, u16Addr, len));

	HAL_ONIF_Switch_Pad();

	HAL_ONIF_RW_Init(RW_MODE_ASYNC);

	REG_WRITE_UINT16(ONIF_CONFIG0, 0x0044);
	DBG_ONENAND(printk("ONIF_CONFIG0=%04X\n", REG(ONIF_CONFIG0)));

	for(i=0; i<len; i++)
	{
		REG_WRITE_UINT16(ONIF_EVENT, R_CMDQ_EMPTY<<8);
		
		REG_WRITE_UINT16(ONIF_CMDQ_PARAM, u16Addr+i);
		REG_WRITE_UINT16(ONIF_CMDQ_OPCODE, OP_SEND_ADDR);
		REG_WRITE_UINT16(ONIF_CMDQ_OPCODE, OP_ASYNC_READ);

		#if !(defined(USE_POLLING) && USE_POLLING)
		wait_event(onif_wait, (onif_int & R_CMDQ_EMPTY));
		REG_WRITE_UINT16(ONIF_EVENT, 0);
		onif_int = 0;
		#else
		if( HAL_ONIF_Wait_Event(R_CMDQ_EMPTY) )
		{
			printk("Set to async mode timeout\n");
		}
		#endif

		pu16Buf[i] = REG(ONIF_ASYNC_READ_DATA);
	}

	DBG_ONENAND(printk("%s finished, ONIF_CONFIG0=%04X\n", __FUNCTION__, REG(ONIF_CONFIG0)));

	HAL_ONIF_Release_Pad();
}

EXPORT_SYMBOL(HAL_ONIF_Async_Read2Reg);

#if 0
void HAL_ONIF_Sync_Write(U16 *pu16Buf, U16 u16Addr, U32 len)
{
	//U32 i;
	U32 u32DMAAddr = (U32)virt_to_phys(pu16Buf);
	U32 loopcount = len / 256;

	DBG_ONENAND(printk("%s\n", __FUNCTION__));

	#if 1
	_dma_cache_wback_inv((U32)pu16Buf, len*sizeof(unsigned short));	// flush dcache
	#else
	for(i = 0; i < len + CACHE_LINE_SIZE ; i += CACHE_LINE_SIZE)
	{
		cache_op(Hit_Writeback_Inv_D, (U32)pu16Buf + i);
	}
	#endif
    Chip_Flush_Memory();											// flush MIPS write buffer

	REG_WRITE_UINT16(ONIF_CONFIG0, R_SYNC_EN | R_DDIR_W | 0x0044);
	DBG_ONENAND(printk("ONIF_CONFIG0=%04X\n", REG(ONIF_CONFIG0)));

	REG_WRITE_UINT16(ONIF_CMDQ_STATUS, R_CMDQ_EN | R_CMDQ_HOLD);
	DBG_ONENAND(printk("ONIF_CMDQ_STATUS=%04X\n", REG(ONIF_CMDQ_STATUS)));

	REG_WRITE_UINT16(ONIF_EVENT, (R_CMDQ_EMPTY | R_MARKER_CMD_DONE | R_DMA_DATA_END)<<8);
	DBG_ONENAND(printk("ONIF_EVENT=%04X\n", REG(ONIF_EVENT)));

	REG_WRITE_UINT16(ONIF_DMA_START_BYTE_LOW, u32DMAAddr & 0xFFFF);
	DBG_ONENAND(printk("ONIF_DMA_START_BYTE_LOW=%04X\n", REG(ONIF_DMA_START_BYTE_LOW)));

	REG_WRITE_UINT16(ONIF_DMA_START_BYTE_HIGH, u32DMAAddr >> 16);
	DBG_ONENAND(printk("ONIF_DMA_START_BYTE_HIGH=%04X\n", REG(ONIF_DMA_START_BYTE_HIGH)));

	REG_WRITE_UINT16(ONIF_DMA_PACKET_SIZE, 512);
	DBG_ONENAND(printk("ONIF_DMA_PACKET_SIZE=%04X\n", REG(ONIF_DMA_PACKET_SIZE)));

	#if defined(SEPARATE_DMA_CTRL) && SEPARATE_DMA_CTRL
	REG_WRITE_UINT16(ONIF_DMA_CTRL, loopcount);
	#endif

	REG_WRITE_UINT16(ONIF_DMA_CTRL, R_DMA_EN | loopcount);
	DBG_ONENAND(printk("ONIF_DMA_CTRL=%04X\n", REG(ONIF_DMA_CTRL)));

	#if defined(SYNC_RW_MAX_512) && SYNC_RW_MAX_512
	for(i=0; i<loopcount-1; i++)
	{
		REG_WRITE_UINT16(ONIF_CMDQ_PARAM, 0x0003);
		REG_WRITE_UINT16(ONIF_CMDQ_OPCODE, OP_WAIT_STATUS);
		REG_WRITE_UINT16(ONIF_CMDQ_PARAM, u16Addr+i*0x100);
		REG_WRITE_UINT16(ONIF_CMDQ_OPCODE, (0x40<<4)|OP_SEND_ADDR);
		REG_WRITE_UINT16(ONIF_CMDQ_PARAM, 255);
		REG_WRITE_UINT16(ONIF_CMDQ_OPCODE, OP_SYNC_READ_WRITE);
	}
	REG_WRITE_UINT16(ONIF_CMDQ_PARAM, 0x0003);
	REG_WRITE_UINT16(ONIF_CMDQ_OPCODE, OP_WAIT_STATUS);
	REG_WRITE_UINT16(ONIF_CMDQ_PARAM, u16Addr+(loopcount-1)*0x100);
	REG_WRITE_UINT16(ONIF_CMDQ_OPCODE, (0x40<<4)|OP_SEND_ADDR);
	REG_WRITE_UINT16(ONIF_CMDQ_PARAM, 255);
	REG_WRITE_UINT16(ONIF_CMDQ_OPCODE, R_CMD_MARKER | OP_SYNC_READ_WRITE);
	#else
	REG_WRITE_UINT16(ONIF_CMDQ_PARAM, 0x0003);
	REG_WRITE_UINT16(ONIF_CMDQ_OPCODE, OP_WAIT_STATUS);
	REG_WRITE_UINT16(ONIF_CMDQ_PARAM, u16Addr);
	REG_WRITE_UINT16(ONIF_CMDQ_OPCODE, (0x40<<4)|OP_SEND_ADDR);
	REG_WRITE_UINT16(ONIF_CMDQ_PARAM, len-1);
	REG_WRITE_UINT16(ONIF_CMDQ_OPCODE, R_CMD_MARKER | OP_SYNC_READ_WRITE);
	#endif

	REG_WRITE_UINT16(ONIF_CMDQ_STATUS, R_CMDQ_EN);
	DBG_ONENAND(printk("ONIF_CMDQ_STATUS=%04X\n", REG(ONIF_CMDQ_STATUS)));

	if( HAL_ONIF_Wait_Event(R_DMA_DATA_END | R_CMDQ_EMPTY | R_MARKER_CMD_DONE, 1000) )
		while(1);

	DBG_ONENAND(printk("%s finished, ONIF_CONFIG0=%04X\n", __FUNCTION__, REG(ONIF_CONFIG0)));

}

EXPORT_SYMBOL(HAL_ONIF_Sync_Write);
#endif

void HAL_ONIF_Sync_Read(U16 *pu16Buf, U16 u16Addr, U32 len)
{
	U32 u32DMAAddr = (U32)virt_to_phys(pu16Buf);
	U32 loopcount;
	int lastdonezcount = 0;

	HAL_ONIF_Switch_Pad();

	DBG_ONENAND(printk("%s\n", __FUNCTION__));

	_dma_cache_wback_inv((U32)pu16Buf, len*sizeof(unsigned short));	// flush dcache
    //Chip_Flush_Memory();											// flush MIPS write buffer

	HAL_ONIF_RW_Init(RW_MODE_SYNC);

	REG_WRITE_UINT16(ONIF_CONFIG0, R_SYNC_EN | 0x0044);
	DBG_ONENAND(printk("ONIF_CONFIG0=%04X\n", REG(ONIF_CONFIG0)));

	REG_WRITE_UINT16(ONIF_CMDQ_STATUS, R_CMDQ_EN | R_CMDQ_HOLD);
	DBG_ONENAND(printk("ONIF_CMDQ_STATUS=%04X\n", REG(ONIF_CMDQ_STATUS)));

	#if defined(USE_POLLING) && USE_POLLING
	REG_WRITE_UINT16(ONIF_EVENT, 0);
	#else
	REG_WRITE_UINT16(ONIF_EVENT, (R_CMDQ_EMPTY | R_MARKER_CMD_DONE | R_DMA_DATA_END)<<8);
	#endif
	DBG_ONENAND(printk("ONIF_EVENT=%04X\n", REG(ONIF_EVENT)));

	REG_WRITE_UINT16(ONIF_DMA_START_BYTE_LOW, u32DMAAddr & 0xFFFF);
	DBG_ONENAND(printk("ONIF_DMA_START_BYTE_LOW=%04X\n", REG(ONIF_DMA_START_BYTE_LOW)));

	REG_WRITE_UINT16(ONIF_DMA_START_BYTE_HIGH, u32DMAAddr >> 16);
	DBG_ONENAND(printk("ONIF_DMA_START_BYTE_HIGH=%04X\n", REG(ONIF_DMA_START_BYTE_HIGH)));

	if( len >= 256 )
	{
		REG_WRITE_UINT16(ONIF_DMA_PACKET_SIZE, 512);
		loopcount = len/256;
	}
	else
	{
		REG_WRITE_UINT16(ONIF_DMA_PACKET_SIZE, len*2);
		loopcount = 1;
	}
	DBG_ONENAND(printk("ONIF_DMA_PACKET_SIZE=%04X\n", REG(ONIF_DMA_PACKET_SIZE)));

	REG_WRITE_UINT16(ONIF_DMA_CTRL, R_DMA_EN | loopcount);
	DBG_ONENAND(printk("ONIF_DMA_CTRL=%04X\n", REG(ONIF_DMA_CTRL)));

	REG_WRITE_UINT16(ONIF_CMDQ_PARAM, 0x0003);
	REG_WRITE_UINT16(ONIF_CMDQ_OPCODE, OP_WAIT_STATUS);
	REG_WRITE_UINT16(ONIF_CMDQ_PARAM, u16Addr);
	REG_WRITE_UINT16(ONIF_CMDQ_OPCODE, OP_SEND_ADDR);
	REG_WRITE_UINT16(ONIF_CMDQ_PARAM, len-1);
	REG_WRITE_UINT16(ONIF_CMDQ_OPCODE, R_CMD_MARKER | OP_SYNC_READ_WRITE);

	REG_WRITE_UINT16(ONIF_CMDQ_STATUS, R_CMDQ_EN);
	DBG_ONENAND(printk("ONIF_CMDQ_STATUS=%04X\n", REG(ONIF_CMDQ_STATUS)));

	#if !(defined(USE_POLLING) && USE_POLLING)
	wait_event(onif_wait, (onif_int & (R_CMDQ_EMPTY)));
	DBG_ONENAND(printk("wait cmdq done\n"));
	wait_event(onif_wait, (onif_int & (R_MARKER_CMD_DONE)));
	DBG_ONENAND(printk("wait marker done\n"));
	wait_event(onif_wait, (onif_int & (R_DMA_DATA_END)));
	DBG_ONENAND(printk("wait dataend done\n"));
	REG_WRITE_UINT16(ONIF_EVENT, 0);
	onif_int = 0;
	#else

	if( HAL_ONIF_Wait_Event(R_CMDQ_EMPTY | R_MARKER_CMD_DONE | R_DMA_DATA_END) )
	{
		printk("%s: Wait event timeout!\n", __FUNCTION__);
	}
	
	#endif

	// we need to check the last done Z
	while( !(REG(ONIF_MIU_CTRL) & R_MIU_LAST_DONE_STATUS) )
	{
		if( ++lastdonezcount > 0xFFFF )
		{
			printk("wait last done Z status timeout\n");
			break;
		}
	}

	//Chip_Read_Memory();

	HAL_ONIF_Release_Pad();

	DBG_ONENAND(printk("%s finished, ONIF_CONFIG0=%04X\n", __FUNCTION__, REG(ONIF_CONFIG0)));
}

EXPORT_SYMBOL(HAL_ONIF_Sync_Read);

void HAL_ParFlash_Reads(U32 u32Addr, U8 *pu8Data, U32 u32Size)
{
        // Removed by Mstar, useless code
	NC_LOCK_FCIE();

	HAL_ONIF_Sync_Read((U16*)pu8Data, ((U32)u32Addr&0x1FFFF)>>1, u32Size>>1);

	NC_UNLOCK_FCIE();
}

EXPORT_SYMBOL(HAL_ParFlash_Reads);

void HAL_ParFlash_Writes(U32 u32Addr, const U8 *pu8Data, U32 u32Size)
{
	NC_LOCK_FCIE();

	HAL_ONIF_Async_Write((U16*)pu8Data, ((u32Addr&0x1FFFF)/2), u32Size/2);

	NC_UNLOCK_FCIE();
}

EXPORT_SYMBOL(HAL_ParFlash_Writes);


#ifdef __cplusplus
}
#endif

