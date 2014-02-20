#ifndef NAND_DRV_LINUX_H
#define NAND_DRV_LINUX_H

#include <linux/delay.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <asm/io.h>

#define REG_BANK_CLKGEN						(0x580)
#define REG_BANK_CHIPTOP					(0xF00)
#define REG_BANK_CHIPGPIO					(0x1580)
#define REG_BANK_FCIE0						(0x8980)
#define REG_BANK_FCIE2						(0x8A00)

#define REG_BANK_PM_GPIO					(0x780)

#define KSEG02KSEG1(addr)					((void *)((U32)(addr)|0x20000000))

#define NAND_MIPS_WRITE_BUFFER_PATCH		1
#define NAND_MIPS_READ_BUFFER_PATCH			1

extern void Chip_Flush_Memory(void);
extern void Chip_Read_Memory(void);

//=====================================================
// HW parameters
//=====================================================
#define REG(Reg_Addr)						(*(volatile U16*)(Reg_Addr))
#define REG_OFFSET_SHIFT_BITS				2 

//----------------------------------
// should only one be selected
#define NC_SEL_FCIE3						1 

#define IF_IP_VERIFY						0 // [CAUTION]: to verify IP and HAL code, defaut 0
//----------------------------------

#define RIU_NONPM_BASE						0xBF000000
#define RIU_BASE							0xBF200000

#define MPLL_CLK_REG_BASE_ADDR				(RIU_BASE+(REG_BANK_CLKGEN<<REG_OFFSET_SHIFT_BITS))
#define CHIPTOP_REG_BASE_ADDR				(RIU_BASE+(REG_BANK_CHIPTOP<<REG_OFFSET_SHIFT_BITS))
#define FCIE_REG_BASE_ADDR					(RIU_BASE+(REG_BANK_FCIE0<<REG_OFFSET_SHIFT_BITS))
#define FCIE_NC_CIFD_BASE					(RIU_BASE+(REG_BANK_FCIE2<<REG_OFFSET_SHIFT_BITS))
#define PM_GPIO_BASE						(RIU_NONPM_BASE+(REG_BANK_PM_GPIO<<REG_OFFSET_SHIFT_BITS))
#define CHIPGIO_REG_BASE_ADDR				(RIU_BASE+(REG_BANK_CHIPGPIO<<REG_OFFSET_SHIFT_BITS))

#include "drvNand_reg_v3.h" 

#define REG_WRITE_UINT16(reg_addr, val)		REG(reg_addr) = val
#define REG_READ_UINT16(reg_addr, val)		val = REG(reg_addr)
#define HAL_WRITE_UINT16(reg_addr, val)		(REG(reg_addr) = val)
#define HAL_READ_UINT16(reg_addr, val)		val = REG(reg_addr)
#define REG_SET_BITS_UINT16(reg_addr, val)	REG(reg_addr) |= (val)
#define REG_CLR_BITS_UINT16(reg_addr, val)	REG(reg_addr) &= ~(val)
#define REG_W1C_BITS_UINT16(reg_addr, val)	REG_WRITE_UINT16(reg_addr, REG(reg_addr)&(val))

//=====================================================
// Nand Driver configs
//=====================================================
#define NAND_DRIVER_OS						1 // for OS / File System
#define NAND_DRIVER_OS_LINUX				(1 & NAND_DRIVER_OS)

//-------------------------------
#define NAND_ENV_FPGA						1
#define NAND_ENV_ASIC						2
#define NAND_DRIVER_ENV						NAND_ENV_ASIC /* [CAUTION] */

//=====================================================
// debug option
//=====================================================
#define NAND_TEST_IN_DESIGN					0 

#define NAND_DEBUG_MSG						0

#if defined(NAND_DEBUG_MSG) && NAND_DEBUG_MSG
#define nand_printf	printk
#define NANDMSG(x)	(x)
#else
#define nand_printf	printk
#define NANDMSG(x)
#endif

//=====================================================
// HW Timer for Delay 
//=====================================================
#define HW_TIMER_DELAY_1us					1
#define HW_TIMER_DELAY_1ms					1000
#define HW_TIMER_DELAY_1s					1000000

/*return Timer tick*/
U32 nand_hw_timer_delay(U32 u32usTick);

//=====================================================
// Pads Switch
//=====================================================
#define CHIPTOP_REG_BASE_ADDR				(RIU_BASE+(REG_BANK_CHIPTOP<<REG_OFFSET_SHIFT_BITS))

/*set bit 4, 7, 8; clr bit 5*/
#define reg_nf_en							GET_REG_ADDR(CHIPTOP_REG_BASE_ADDR, 0x6f)

/*clr bit 15*/
#define reg_allpad_in						GET_REG_ADDR(CHIPTOP_REG_BASE_ADDR, 0x50)

#define IF_FCIE_SHARE_PINS					1	// 1: need to nand_pads_switch at HAL's functions.
												// 0: only nand_pads_switch in drvNand_Init.
#define NAND_CE_GPIO						1
#define reg_gpio_pm_oen						GET_REG_ADDR(PM_GPIO_BASE, 0x08)
#define NAND_WP_GPIO						1
#define reg_ts0_gpio_out01					GET_REG_ADDR(CHIPGIO_REG_BASE_ADDR, 0x0E)


/*
  if pin-shared with Card IF, need to call before every JOB_START.
*/
U32 nand_pads_switch(U32 u32EnableFCIE);

//=====================================================
// set FCIE clock
//=====================================================
#define reg_ckg_fcie						GET_REG_ADDR(MPLL_CLK_REG_BASE_ADDR, 0x64)

#define NFIE_CLK_MASK						(BIT4|BIT3|BIT2)
#define NFIE_CLK_5_4M						(0)
#define NFIE_CLK_27M						(BIT2)
#define NFIE_CLK_32M						(BIT3)
#define NFIE_CLK_43M						(BIT3|BIT2)
#define NFIE_CLK_86M						(BIT4)

U32 nand_clock_setting(U32 u32ClkParam);

//=====================================================
// transfer DMA Address
//=====================================================
// return DMA address for FCIE3 registers
U32 nand_translate_DMA_address_Ex(U32 u32_DMAAddr, U32 u32_ByteCnt);
U32 nand_get_internal_DMA_address_Ex(U32 u32DMAAddr);

//=====================================================
// reset WatchDog
//=====================================================
void nand_reset_WatchDog(void);

//=====================================================
// misc
//=====================================================
//#define BIG_ENDIAN
#define LITTLE_ENDIAN

typedef struct _NAND_DRIVER_PLATFORM_DEPENDENT
{
	U8 *pu8_Spare;	
}NAND_DRIVER_PLAT, *P_NAND_DRIVER_PLAT;

U32 nand_platform_init(void);

void drvNAND_SetCEGPIO(U8 u8Level);
void drvNAND_SetWPGPIO(U8 u8Level);

#endif
