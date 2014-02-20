//=======================================================================
//  MStar Semiconductor - Unified Nand Flash Driver
//
//  drvNand_platform.c - Storage Team, 2009/08/20 
//
//  Design Notes: defines common platform-dependent functions.
//
//=======================================================================
#include "../include/drvNand.h"

#if defined(NAND_DRV_LINUX) && NAND_DRV_LINUX

U32 nand_hw_timer_delay(U32 u32usTick)
{
	udelay(u32usTick);
	return u32usTick;
}

//=====================================================
// Pads Switch
//=====================================================

// if pin-shared with Card IF, need to call before every JOB_START.
U32 nand_pads_switch(U32 u32EnableFCIE)
{
	#if defined(NAND_DEBUG_MSG) && NAND_DEBUG_MSG
	U16 u16Tmp;
	#endif

	if(u32EnableFCIE)
	{
		#if defined(CONFIG_MSTAR_TITANIA8)
		REG_SET_BITS_UINT16(reg_nf_en, BIT7);
		REG_CLR_BITS_UINT16(reg_allpad_in, BIT15);
		#elif defined(CONFIG_MSTAR_TITANIA9)
		REG_SET_BITS_UINT16(reg_nf_en, BIT4);
		REG_CLR_BITS_UINT16(reg_allpad_in, BIT15);
		#endif
	}

	#if defined(NAND_DEBUG_MSG) && NAND_DEBUG_MSG
	REG_READ_UINT16(reg_nf_en, u16Tmp);
	NANDMSG(nand_printf("reg_nf_en(%08X)=%04X\n", reg_nf_en, u16Tmp));
	REG_READ_UINT16(reg_allpad_in, u16Tmp);
	NANDMSG(nand_printf("reg_allpad_in(%08X)=%04X\n", reg_allpad_in, u16Tmp));
	#endif

	REG_WRITE_UINT16(NC_PATH_CTL, BIT_NC_EN);

	return 0; // ok
}

//=====================================================
// set FCIE clock
//=====================================================
U32 nand_clock_setting(U32 u32ClkParam)
{
	#if defined(NAND_DEBUG_MSG) && NAND_DEBUG_MSG
	U16 u16Tmp;
	#endif

	REG_CLR_BITS_UINT16(reg_ckg_fcie, BIT6-1); // enable FCIE clk, set to lowest clk
	REG_SET_BITS_UINT16(reg_ckg_fcie, u32ClkParam);
	
	#if defined(NAND_DEBUG_MSG) && NAND_DEBUG_MSG
	REG_READ_UINT16(reg_ckg_fcie, u16Tmp);
	NANDMSG(nand_printf("reg_ckg_fcie(%08X)=%08X\n", reg_ckg_fcie, u16Tmp));
	#endif
	
	return 0;
}

U32 nand_translate_DMA_address_Ex(U32 u32_DMAAddr, U32 u32_ByteCnt)
{
	return u32_DMAAddr;
}

void nand_reset_WatchDog(void)
{
}

U32 nand_platform_init(void)
{
	return UNFD_ST_SUCCESS;
}

#else

#error "Error! no platform functions."

#endif
