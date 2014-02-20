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

#ifndef _MHAL_ONIF_REG_H_
#define _MHAL_ONIF_REG_H_

#include "mdrv_types.h"

//-------------------------------------------------------------------------------------------------
//  Macro and Define
//-------------------------------------------------------------------------------------------------
#define DBG_ONENAND(x)						//(x)

#define REG(Reg_Addr)						(*(volatile U16*)(Reg_Addr))
#define REG_OFFSET_SHIFT_BITS				2

#define REG_WRITE_UINT16(reg_addr, val)		REG(reg_addr) = val
#define REG_READ_UINT16(reg_addr, val)		val = REG(reg_addr)
#define HAL_WRITE_UINT16(reg_addr, val)		(REG(reg_addr) = val)
#define HAL_READ_UINT16(reg_addr, val)		val = REG(reg_addr)
#define REG_SET_BITS_UINT16(reg_addr, val)	REG(reg_addr) |= (val)
#define REG_CLR_BITS_UINT16(reg_addr, val)	REG(reg_addr) &= ~(val)
#define REG_W1C_BITS_UINT16(reg_addr, val)	REG_WRITE_UINT16(reg_addr, REG(reg_addr)&(val))

#define RIUBASE								0xBF000000

#define REG_BANK_PMSLEEP					(0x700)
#define REG_BANK_TIMER0						(0x1800)
#define REG_BANK_CLKGEN						(0x80580)
#define REG_BANK_CHIPTOP					(0x80F00)
#define REG_BANK_FCIE3						(0x88980)
#define REG_BANK_ONIF						(0x88B00)

#define PM_SLEEP_BASE						(RIUBASE+(REG_BANK_PMSLEEP<<REG_OFFSET_SHIFT_BITS))
#define TIMER0_BASE							(RIUBASE+(REG_BANK_TIMER0<<REG_OFFSET_SHIFT_BITS))
#define CLKGEN_BASE							(RIUBASE+(REG_BANK_CLKGEN<<REG_OFFSET_SHIFT_BITS))
#define CHIPTOP_BASE						(RIUBASE+(REG_BANK_CHIPTOP<<REG_OFFSET_SHIFT_BITS))
#define FCIE3_BASE							(RIUBASE+(REG_BANK_FCIE3<<REG_OFFSET_SHIFT_BITS))
#define ONIF_BASE							(RIUBASE+(REG_BANK_ONIF<<REG_OFFSET_SHIFT_BITS))

// ONIF Registers
#define ONIF_CONFIG0						(ONIF_BASE+(0x00<<REG_OFFSET_SHIFT_BITS))
#define ONIF_ASYNC_READ_DATA				(ONIF_BASE+(0x01<<REG_OFFSET_SHIFT_BITS))
#define ONIF_CONFIG1						(ONIF_BASE+(0x02<<REG_OFFSET_SHIFT_BITS))
#define ONIF_CMDQ_PARAM						(ONIF_BASE+(0x03<<REG_OFFSET_SHIFT_BITS))
#define ONIF_CMDQ_OPCODE					(ONIF_BASE+(0x04<<REG_OFFSET_SHIFT_BITS))
#define ONIF_CMDQ_STATUS					(ONIF_BASE+(0x05<<REG_OFFSET_SHIFT_BITS))
#define ONIF_EVENT							(ONIF_BASE+(0x06<<REG_OFFSET_SHIFT_BITS))
#define ONIF_DMA_STATUS0					(ONIF_BASE+(0x07<<REG_OFFSET_SHIFT_BITS))
#define ONIF_DMA_STATUS1					(ONIF_BASE+(0x08<<REG_OFFSET_SHIFT_BITS))
#define ONIF_INDIR_ACCESS_START_ADDR		(ONIF_BASE+(0x09<<REG_OFFSET_SHIFT_BITS))
#define ONIF_INDIR_ACCESS_START_DATA		(ONIF_BASE+(0x0A<<REG_OFFSET_SHIFT_BITS))
#define ONIF_ON_BOOTRAM_START_ADDR			(ONIF_BASE+(0x0B<<REG_OFFSET_SHIFT_BITS))
#define ONIF_ON_DATARAM_START_ADDR			(ONIF_BASE+(0x0C<<REG_OFFSET_SHIFT_BITS))
#define ONIF_DMA_START_BYTE_LOW				(ONIF_BASE+(0x0D<<REG_OFFSET_SHIFT_BITS))
#define ONIF_DMA_START_BYTE_HIGH			(ONIF_BASE+(0x0E<<REG_OFFSET_SHIFT_BITS))
#define ONIF_DMA_PACKET_SIZE				(ONIF_BASE+(0x0F<<REG_OFFSET_SHIFT_BITS))
#define ONIF_DMA_CTRL						(ONIF_BASE+(0x10<<REG_OFFSET_SHIFT_BITS))
#define ONIF_MIU_CTRL						(ONIF_BASE+(0x11<<REG_OFFSET_SHIFT_BITS))
#define ONIF_MIU_LOW_BOUND_LOW				(ONIF_BASE+(0x12<<REG_OFFSET_SHIFT_BITS))
#define ONIF_MIU_LOW_BOUND_HIGH				(ONIF_BASE+(0x13<<REG_OFFSET_SHIFT_BITS))
#define ONIF_MIU_HIGH_BOUND_LOW				(ONIF_BASE+(0x14<<REG_OFFSET_SHIFT_BITS))
#define ONIF_MIU_HIGH_BOUND_HIGH			(ONIF_BASE+(0x15<<REG_OFFSET_SHIFT_BITS))
#define ONIF_MIU_ERR_ADDR_LOW				(ONIF_BASE+(0x16<<REG_OFFSET_SHIFT_BITS))
#define ONIF_MIU_ERR_ADDR_HIGH				(ONIF_BASE+(0x17<<REG_OFFSET_SHIFT_BITS))
#define ONIF_REMAIN_PACKETS					(ONIF_BASE+(0x18<<REG_OFFSET_SHIFT_BITS))
#define ONIF_DATA_BUS_VALUE					(ONIF_BASE+(0x19<<REG_OFFSET_SHIFT_BITS))
#define ONIF_DBUS_DEBUG						(ONIF_BASE+(0x20<<REG_OFFSET_SHIFT_BITS))
#define ONIF_DBUS_DEBUG_SEL					(ONIF_BASE+(0x21<<REG_OFFSET_SHIFT_BITS))
#define ONIF_MIU_PACTH						(ONIF_BASE+(0x24<<REG_OFFSET_SHIFT_BITS))

// PMSLEEP Registers
#define CHIP_CONFIG_OVERWRITE				(PM_SLEEP_BASE+(0x1F<<REG_OFFSET_SHIFT_BITS))

// CHIPTOP Registers
#define REG_SECONDUARTMODE					(CHIPTOP_BASE+(0x02<<REG_OFFSET_SHIFT_BITS))
#define REG_ALLPAD_IN						(CHIPTOP_BASE+(0x50<<REG_OFFSET_SHIFT_BITS))
#define REG_SPDIFOUTCONFIG					(CHIPTOP_BASE+(0x57<<REG_OFFSET_SHIFT_BITS))
#define REG_PCMADCONFIG						(CHIPTOP_BASE+(0x64<<REG_OFFSET_SHIFT_BITS))
#define REG_PCMCONFIG						(CHIPTOP_BASE+(0x6E<<REG_OFFSET_SHIFT_BITS))
#define REG_NAND_MODE						(CHIPTOP_BASE+(0x6F<<REG_OFFSET_SHIFT_BITS))

// CLKGEN Registers
#define REG_CLK_MCU							(CLKGEN_BASE+(0x10<<REG_OFFSET_SHIFT_BITS))
#define REG_CLK_ONIF						(CLKGEN_BASE+(0x66<<REG_OFFSET_SHIFT_BITS))

// FCIE3 Registers
#define REG_NC_EN							(FCIE3_BASE+(0x0A<<REG_OFFSET_SHIFT_BITS))

//Timer0 Registers
#define TIMER0_ENABLE						(TIMER0_BASE+(0x10<<REG_OFFSET_SHIFT_BITS))
#define TIMER0_HIT							(TIMER0_BASE+(0x11<<REG_OFFSET_SHIFT_BITS))
#define TIMER0_MAX_LOW						(TIMER0_BASE+(0x12<<REG_OFFSET_SHIFT_BITS))
#define TIMER0_MAX_HIGH						(TIMER0_BASE+(0x13<<REG_OFFSET_SHIFT_BITS))
#define TIMER0_CAP_LOW						(TIMER0_BASE+(0x14<<REG_OFFSET_SHIFT_BITS))
#define TIMER0_CAP_HIGH						(TIMER0_BASE+(0x15<<REG_OFFSET_SHIFT_BITS))


/* ONIF_CONFIG0: 0x00 */
#define R_ONIF_SW_REST						BIT15
#define R_SYNC_DBF_DEST						BIT14
#define R_DBF_INDEN							BIT13
#define R_DBF_RAM							BIT12
#define R_CLK_EN							BIT11
#define R_DDIR_W							BIT10
#define R_SYNC_EN							BIT9
#define R_MUX_EN							BIT8
#define R_ASYN_RDEST						BIT7
#define R_PIN_RPZ							BIT6
#define R_PIN_RDY							BIT5
#define R_PIN_INT							BIT4

/* ONIF_CONFIG1: 0x02 */
#define R_AVD_WRITE_DISABLE					BIT11
#define R_ASYNC_AVD_WIDTH_MASK				(BIT10 | BIT9 | BIT8)
#define R_ASYNC_OE_WIDTH_MASK				(BIT6 | BIT5 | BIT4)
#define R_ASYNC_WE_WIDTH_MASK				(BIT2 | BIT1 | BIT0)

/* ONIF_CMDQ_OPCODE: 0x04 */
#define OP_SEND_ADDR						0x01
#define OP_CHG_MODE							0x03
#define OP_ASYNC_READ						0x04
#define OP_ASYNC_WRITE						0x05
#define OP_SYNC_READ_WRITE					0x06
#define OP_WAIT_STATUS						0x08
#define OP_DELAY_TICK						0x0A
#define R_CMD_MARKER						BIT4

/* ONIF_CMDQ_STATUS: 0x05 */
#define R_CMDQ_REPAET_EN					BIT7
#define R_CMDQ_HOLD							BIT6
#define R_CMDQ_EN							BIT5

/* ONIF_EVENT: 0x06 */
#define R_CMDQ_EMPTY						BIT0
#define R_MARKER_CMD_DONE					BIT1
#define R_DMA_DATA_END						BIT2
#define R_MIU_OUT_OF_RANGE					BIT3
#define R_SYND_OBBWL						BIT4

/* ONIF_INDIR_ACCESS_START_ADDR: 0x09 */
#define R_DBF_READY							BIT12

/* ONIF_DMA_CTRL: 0x10 */
#define R_DMA_EN							BIT12
#define R_BUF_READY_CTRL					BIT13

/* ONIF_MIU_CTRL: 0x11 */
#define R_MIU_SELECT						BIT0
#define R_MIU_REQUEST_RESET					BIT1
#define R_MIU_BUS_TYPE_MASK					(BIT3 | BIT2)
#define R_MIU_BUS_CTRL						BIT4
#define R_MIU_WP_RANGE_EN					BIT5
#define R_PING_PONG_FIFO_CLK_EN				BIT6
#define R_MIU_LAST_DONE_STATUS				BIT7

/* ONIF_DBUS_DEBUG_SEL: 0x11 */
#define R_DEB_SEL_MASK						(BIT10 | BIT9 | BIT8)
#define R_DEB_SEL_DISABLE					((0x00)<<8)
#define R_DEB_SEL_MMA						((0x01)<<8)
#define R_DEB_SEL_DFF						((0x02)<<8)
#define R_DEB_SEL_ONB						((0x03)<<8)

/* ONIF Clock */
#define ONIF_CLK_DISABLE					BIT0
#define ONIF_CLK_XTAL						(0)
#define ONIF_CLK_5M							(0x1<<2)
#define ONIF_CLK_27M						(0x2<<2)
#define ONIF_CLK_54M						(0x3<<2)
#define ONIF_CLK_72M						(0x4<<2)
#define ONIF_CLK_86M						(0x5<<2)
#define ONIF_CLK_43M						(0x6<<2)
#define ONIF_CLK_96M						(0x7<<2)
#define ONIF_CLK_SEL						BIT5

#define USE_POLLING							1

#if USE_POLLING
#define ONIF_WAIT_OK						0
#define ONIF_WAIT_TIMEOUT					1
#endif

#define ASYNC_READ_RIU						0
#define SYNC_WRITE							0

#define RW_MODE_ASYNC						0
#define RW_MODE_SYNC						1
#define RW_MODE_NONE						2


//-------------------------------------------------------------------------------------------------
//  Type and Structure
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Function and Variable
//-------------------------------------------------------------------------------------------------


#endif // _MHAL_ONENAND_REG_H_
