//=======================================================================
//  MStar Semiconductor - Unified Nand Flash Driver
//
//  drvNand_hal_v3.c - Storage Team, 2009/08/20
//
//  Design Notes:
//    2009/08/21 - FCIE3 physical layer functions
//
//=======================================================================

#include "../include/drvNand.h"
//#include "../../../drivers/mstar/onenand/fcie4/mhal_onenand_reg.h"



#if IF_FCIE_SHARE_PINS
  #define NC_PAD_SWITCH(enable)    nand_pads_switch(enable);
#else
  #define NC_PAD_SWITCH(enable)    // NULL to save CPU a JMP/RET time
#endif

#if NC_SEL_FCIE3
//========================================================
// static functions
//========================================================
U32  NC_ResetFCIE(void);
static U32  NC_CheckEWStatus(U8 u8_OpType);
U32  NC_WaitComplete(U16 u16_WaitEvent, U32 u32_MicroSec);

#define OPTYPE_ERASE        1
#define OPTYPE_WRITE        2

//========================================================
// function definitions
//========================================================
#if defined(CONFIG_MSTAR_TITANIA8)
DECLARE_MUTEX(FCIE3_mutex);

void nand_lock_fcie(void)
{
	down(&FCIE3_mutex);
}

void nand_unlock_fcie(void)
{
	up(&FCIE3_mutex);
}

#define NC_LOCK_FCIE()		nand_lock_fcie()
#define NC_UNLOCK_FCIE()	nand_unlock_fcie()

#else

#define NC_LOCK_FCIE()
#define NC_UNLOCK_FCIE()

#endif

U32 NC_ResetFCIE(void)
{
	volatile U16 u16Reg;

	NC_PAD_SWITCH(1);

    REG_WRITE_UINT16(NC_SIGNAL, 0);
    REG_WRITE_UINT16(NC_WIDTH, 0);
    REG_WRITE_UINT16(NC_PATH_CTL, 0);
    REG_WRITE_UINT16(NC_CTRL, 0);

    REG_READ_UINT16(NC_MIE_EVENT, u16Reg);
    REG_WRITE_UINT16(NC_MIE_EVENT, u16Reg);

	// soft reset
	REG_CLR_BITS_UINT16(NC_TEST_MODE, BIT_FCIE_SOFT_RST);	/* active low */
	nand_hw_timer_delay(HW_TIMER_DELAY_1us);
	REG_SET_BITS_UINT16(NC_TEST_MODE, BIT_FCIE_SOFT_RST);
	nand_hw_timer_delay(HW_TIMER_DELAY_1us);

	REG_READ_UINT16(NC_SIGNAL, u16Reg);
	if(DEF_REG0x40_VAL != u16Reg)
	{
		NANDMSG(nand_printf("ERROR: NC_ResetFCIE, ErrCode:%Xh \r\n", UNFD_ST_ERR_NO_NFIE));
		return UNFD_ST_ERR_NO_NFIE;
	}

    REG_SET_BITS_UINT16(NC_PATH_CTL, BIT_NC_EN);
    REG_SET_BITS_UINT16(NC_SIGNAL, BIT_NC_CHK_RB_HIGH);

	return UNFD_ST_SUCCESS; // ok
}

/*-----------------------------
  config HAL parameters
  -----------------------------*/
U32 NC_Init(void)
{
	U32 u32_RetVal;
    NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNand_get_DrvContext_address();

    // disable NC
    REG_CLR_BITS_UINT16(NC_PATH_CTL, BIT_NC_EN);
	REG_WRITE_UINT16(NC_CTRL , 0);
	// reset NC
	u32_RetVal = NC_ResetFCIE();
	if(UNFD_ST_SUCCESS != u32_RetVal)
	{
		NANDMSG(nand_printf("ERROR: NC_Init, ErrCode:%Xh \r\n", UNFD_ST_ERR_NO_NFIE));
		return u32_RetVal;
	}
	// disable interupts
	REG_CLR_BITS_UINT16(NC_MIE_INT_EN, BIT_MMA_DATA_END | BIT_NC_JOB_END);
    // clean int events
	REG_W1C_BITS_UINT16(NC_MIE_EVENT, BIT_MMA_DATA_END | BIT_NC_JOB_END);
    // enable NC
	REG_SET_BITS_UINT16(NC_PATH_CTL, BIT_NC_EN);

	// config NC
	pNandDrv->u16_Reg1B_SdioCtrl = pNandDrv->u32_SectorByteCnt +
	    ((pNandDrv->u16_SpareByteCnt >> pNandDrv->u8_PageSectorCntBits) & (~(U32)1));
	if(NC_MAX_SECTOR_BYTE_CNT < pNandDrv->u16_Reg1B_SdioCtrl)
	{
		NANDMSG(nand_printf("ERROR: invalid Sector Size: %Xh bytes!\r\n", pNandDrv->u16_Reg1B_SdioCtrl));
		return UNFD_ST_ERR_HAL_R_INVALID_PARAM;
	}
	pNandDrv->u16_Reg1B_SdioCtrl |= BIT_SDIO_BLK_MODE;

	pNandDrv->u16_Reg40_Signal =
		(BIT_NC_CE_AUTO|BIT_NC_CE_H|BIT_NC_WP_AUTO|BIT_NC_WP_H) &
		~(BIT_NC_CHK_RB_EDGEn | BIT_NC_CE_SEL_MASK);
	
	#if defined(NAND_CE_GPIO) && NAND_CE_GPIO
	if( pNandDrv->u16_Reg40_Signal & BIT_NC_CE_H)
		drvNAND_SetCEGPIO(1);
	else
		drvNAND_SetCEGPIO(0);
	#endif

	#if defined(NAND_WP_GPIO) && NAND_WP_GPIO
	REG_SET_BITS_UINT16(ONIF_CONFIG0, R_PIN_RPZ);
	//drvNAND_SetWPGPIO(1);
	#endif
	
	REG_WRITE_UINT16(NC_SIGNAL, pNandDrv->u16_Reg40_Signal);

	pNandDrv->u16_Reg48_Spare = (pNandDrv->u16_SpareByteCnt >> pNandDrv->u8_PageSectorCntBits) & ~(U32)1;
	if(NC_MAX_SECTOR_SPARE_BYTE_CNT < pNandDrv->u16_Reg48_Spare)
	{
		NANDMSG(nand_printf("ERROR: invalid Sector Spare Size: %Xh bytes!\r\n", pNandDrv->u16_Reg48_Spare));
		return UNFD_ST_ERR_HAL_R_INVALID_PARAM;
	}
	#if IF_SPARE_AREA_DMA
	pNandDrv->u16_Reg48_Spare |= BIT_NC_SPARE_DEST_MIU;
	#endif

	if( pNandDrv->u32_PageByteCnt == 512 )
		pNandDrv->u16_Reg48_Spare |= BIT_NC_ONE_COL_ADDR;

	pNandDrv->u16_Reg49_SpareSize = pNandDrv->u16_SpareByteCnt & ~(U32)1;
	if(NC_MAX_TOTAL_SPARE_BYTE_CNT < pNandDrv->u16_Reg49_SpareSize)
	{
		NANDMSG(nand_printf("ERROR: invalid Total Spare Size: %Xh bytes!\r\n", pNandDrv->u16_Reg49_SpareSize));
		return UNFD_ST_ERR_HAL_R_INVALID_PARAM;
	}

	pNandDrv->u16_Reg50_EccCtrl = 0;
	switch(pNandDrv->u32_PageByteCnt)
	{
		case 0x0200:  pNandDrv->u16_Reg50_EccCtrl &= ~BIT_NC_PAGE_SIZE_512Bn;  break;
		case 0x0800:  pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_PAGE_SIZE_2KB;  break;
		case 0x1000:  pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_PAGE_SIZE_4KB;  break;
		case 0x2000:  pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_PAGE_SIZE_8KB;  break;
		case 0x4000:  pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_PAGE_SIZE_16KB;  break;
		case 0x8000:  pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_PAGE_SIZE_32KB;  break;
		default:
			NANDMSG(nand_printf("ERROR: invalid Page Size: %Xh bytes!\r\n", (unsigned int)pNandDrv->u32_PageByteCnt));
			return UNFD_ST_ERR_HAL_R_INVALID_PARAM;

	}
	switch(pNandDrv->u8_ECCType)
	{
		case ECC_TYPE_RS:
			pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_ECC_TYPE_RS;
			pNandDrv->u16_ECCCodeByteCnt = ECC_CODE_BYTECNT_RS;
			break;
		case ECC_TYPE_4BIT:
			pNandDrv->u16_Reg50_EccCtrl &= ~BIT_NC_ECC_TYPE_4b512Bn;
			pNandDrv->u16_ECCCodeByteCnt = ECC_CODE_BYTECNT_4BIT;
		    break;
		case ECC_TYPE_8BIT:
			pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_ECC_TYPE_8b512B;
			pNandDrv->u16_ECCCodeByteCnt = ECC_CODE_BYTECNT_8BIT;
		    break;
		case ECC_TYPE_12BIT:
			pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_ECC_TYPE_12b512B;
			pNandDrv->u16_ECCCodeByteCnt = ECC_CODE_BYTECNT_12BIT;
			break;
		case ECC_TYPE_24BIT1KB:
			pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_ECC_TYPE_24b1KB;
			pNandDrv->u16_ECCCodeByteCnt = ECC_CODE_BYTECNT_24BIT1KB;
			break;
		case ECC_TYPE_32BIT1KB:
			pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_ECC_TYPE_32b1KB;
			pNandDrv->u16_ECCCodeByteCnt = ECC_CODE_BYTECNT_32BIT1KB;
			break;
		default:
			NANDMSG(nand_printf("ERROR: invalid ECC Type: %Xh \r\n", pNandDrv->u8_ECCType));
			return UNFD_ST_ERR_HAL_R_INVALID_PARAM;
			break;
	}
	pNandDrv->u16_Reg50_EccCtrl |= BIT_NC_ECCERR_NSTOP;

	NANDMSG(nand_printf("NAND_Info:\r\n BlkCnt:%Xh  BlkPageCnt:%Xh  PageByteCnt:%Xh  SpareByteCnt:%Xh \r\n",
		(unsigned int)pNandDrv->u16_BlkCnt, (unsigned int)pNandDrv->u16_BlkPageCnt, (unsigned int)pNandDrv->u32_PageByteCnt, (unsigned int)pNandDrv->u16_SpareByteCnt));
	NANDMSG(nand_printf(" BlkPageCntBits:%Xh  PageByteCntBits:%Xh  SpareByteCntBits:%Xh \r\n",
		(unsigned int)pNandDrv->u8_BlkPageCntBits, (unsigned int)pNandDrv->u8_PageByteCntBits, (unsigned int)pNandDrv->u8_SpareByteCntBits));
	NANDMSG(nand_printf(" BlkPageCntMask:%Xh  PageByteCntMask:%Xh  SpareByteCntMask:%Xh \r\n",
		(unsigned int)pNandDrv->u16_BlkPageCntMask, (unsigned int)pNandDrv->u32_PageByteCntMask, pNandDrv->u16_SpareByteCntMask));
	NANDMSG(nand_printf(" PageSectorCnt:%Xh  SectorByteCnt:%Xh  SectorSpareByteCnt:%Xh  ECCCodeByteCnt:%Xh \r\n",
		(unsigned int)pNandDrv->u16_PageSectorCnt, (unsigned int)pNandDrv->u32_SectorByteCnt, (unsigned int)pNandDrv->u16_SectorSpareByteCnt, (unsigned int)pNandDrv->u16_ECCCodeByteCnt));
    NANDMSG(nand_printf(" PageSectorCntBits:%Xh  SectorByteCntBits:%Xh  SectorSpareByteCntBits:%Xh  PageSectorCntMask:%Xh  SectorByteCntMask:%Xh \r\n",
		(unsigned int)pNandDrv->u8_PageSectorCntBits, (unsigned int)pNandDrv->u8_SectorByteCntBits, (unsigned int)pNandDrv->u8_SectorSpareByteCntBits, (unsigned int)pNandDrv->u16_PageSectorCntMask, (unsigned int)pNandDrv->u32_SectorByteCntMask));

	NC_Config();

	#if defined(CONFIG_MSTAR_TITANIA8) || defined(CONFIG_MSTAR_TITANIA9) || defined(CONFIG_MSTAR_AMBER1)
	REG_SET_BITS_UINT16((FCIE_REG_BASE_ADDR+(0x2F<<REG_OFFSET_SHIFT_BITS)), BIT0);
	#endif

	return UNFD_ST_SUCCESS;
}


void NC_Config(void)
{
    NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNand_get_DrvContext_address();

	REG_WRITE_UINT16(NC_SDIO_CTL, pNandDrv->u16_Reg1B_SdioCtrl);
	
#if defined(NAND_CE_GPIO) && NAND_CE_GPIO
	if( pNandDrv->u16_Reg40_Signal & BIT_NC_CE_H)
		drvNAND_SetCEGPIO(1);
	else
		drvNAND_SetCEGPIO(0);
#endif

#if defined(NAND_WP_GPIO) && NAND_WP_GPIO
	if( pNandDrv->u16_Reg40_Signal & BIT_NC_WP_H )
		REG_SET_BITS_UINT16(ONIF_CONFIG0, R_PIN_RPZ);
	//	drvNAND_SetWPGPIO(1);
#endif
	
	REG_WRITE_UINT16(NC_SIGNAL, pNandDrv->u16_Reg40_Signal);
	/*sector spare size*/
	REG_WRITE_UINT16(NC_SPARE, pNandDrv->u16_Reg48_Spare);
	/* page spare size*/
	REG_WRITE_UINT16(NC_SPARE_SIZE, (U16)pNandDrv->u16_Reg49_SpareSize);
	/* page size and ECC type*/
	REG_WRITE_UINT16(NC_ECC_CTRL, pNandDrv->u16_Reg50_EccCtrl);

	NANDMSG(nand_printf("NC_SDIO_CTL: %X \r\n", pNandDrv->u16_Reg1B_SdioCtrl));
	NANDMSG(nand_printf("NC_SIGNAL: %X \r\n", pNandDrv->u16_Reg40_Signal));
	NANDMSG(nand_printf("NC_SPARE: %X \r\n", pNandDrv->u16_Reg48_Spare));
	NANDMSG(nand_printf("NC_SPARE_SIZE: %X \r\n", pNandDrv->u16_Reg49_SpareSize));
	NANDMSG(nand_printf("NC_ECC_CTRL: %X \r\n", pNandDrv->u16_Reg50_EccCtrl));
}

// can not corss block
U32 NC_WritePages
(
    U32 u32_PhyRowIdx, U8 *pu8_DataBuf, U8 *pu8_SpareBuf, U32 u32_PageCnt
)
{
	U16 u16_Tmp;
	//U8 *pu8_Tmp;
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNand_get_DrvContext_address();
	U32 u32_DataDMAAddr;
	#if IF_SPARE_AREA_DMA
	U32 u32_SpareDMAAddr=0;
	#endif

	#if defined(NAND_MIPS_WRITE_BUFFER_PATCH) && NAND_MIPS_WRITE_BUFFER_PATCH
	int i;
	U32 u32Dummy[128];
	U32 *pu32Dummy = u32Dummy;
	#endif

	NANDMSG(printk("%s\n", __FUNCTION__));
	//printk("spare buf[0] %d\n", pu8_SpareBuf[0]);

	// can not corss block
	if((u32_PhyRowIdx & pNandDrv->u16_BlkPageCntMask) + u32_PageCnt >
		pNandDrv->u16_BlkCnt)
	{
		NANDMSG(nand_printf("ERROR: NC_ReadPages, ErrCode:%Xh \r\n", UNFD_ST_ERR_HAL_W_INVALID_PARAM));
		return UNFD_ST_ERR_HAL_W_INVALID_PARAM;
	}

	_dma_cache_wback_inv((U32)pu8_DataBuf, u32_PageCnt*pNandDrv->u32_PageByteCnt);
	_dma_cache_wback_inv((U32)pu8_SpareBuf, u32_PageCnt*pNandDrv->u16_SpareByteCnt);

#if 1
	#if defined(NAND_MIPS_WRITE_BUFFER_PATCH) && NAND_MIPS_WRITE_BUFFER_PATCH
	pu32Dummy = (U32*)KSEG02KSEG1((U32)pu32Dummy);
	for(i=0; i<8; i++)
	{
		pu32Dummy[i*16] = 0x01234567*i;
	}
	#else
	__asm(
			"sync;"
			"nop;"
		 );
	#endif
#endif

	NC_LOCK_FCIE();

	NC_PAD_SWITCH(1);
	REG_WRITE_UINT16(NC_MIE_EVENT, BIT_NC_JOB_END|BIT_MMA_DATA_END);

	u32_DataDMAAddr = nand_translate_DMA_address_Ex((U32)pu8_DataBuf, pNandDrv->u32_PageByteCnt*u32_PageCnt);
	#if IF_SPARE_AREA_DMA
	if(pu8_SpareBuf)
		u32_SpareDMAAddr = nand_translate_DMA_address_Ex((U32)pu8_SpareBuf, pNandDrv->u16_SpareByteCnt);
	else
	{
		for(u16_Tmp=0; u16_Tmp < pNandDrv->u16_PageSectorCnt; u16_Tmp++)
		    memset(pNandDrv->NandDrvPlat_t.pu8_Spare + pNandDrv->u16_SectorSpareByteCnt * u16_Tmp,
		        0xFF, pNandDrv->u16_SectorSpareByteCnt - pNandDrv->u16_ECCCodeByteCnt);
		u32_SpareDMAAddr = nand_translate_DMA_address_Ex((U32)pNandDrv->NandDrvPlat_t.pu8_Spare, pNandDrv->u16_SpareByteCnt);
	}
    REG_WRITE_UINT16(NC_SPARE_DMA_ADR0, u32_SpareDMAAddr & 0xFFFF);
	REG_WRITE_UINT16(NC_SPARE_DMA_ADR1, u32_SpareDMAAddr >>16);
	#endif
	REG_WRITE_UINT16(NC_JOB_BL_CNT, u32_PageCnt << pNandDrv->u8_PageSectorCntBits);
	//REG_WRITE_UINT16(NC_JOB_BL_CNT, 8);
    REG_WRITE_UINT16(NC_SDIO_ADDR0, u32_DataDMAAddr & 0xFFFF);//>>MIU_BUS_WIDTH_BITS));
	REG_WRITE_UINT16(NC_SDIO_ADDR1, u32_DataDMAAddr >> 16);//(MIU_BUS_WIDTH_BITS+16)));
	REG_SET_BITS_UINT16(NC_MMA_PRI_REG, BIT_NC_DMA_DIR_W);
	REG_SET_BITS_UINT16(NC_PATH_CTL, BIT_MMA_EN);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_ADRSET);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PhyRowIdx & 0xFFFF);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PhyRowIdx >> 16);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (pNandDrv->u8_OpCode_RW_AdrCycle<< 8) | (CMD_0x80));
	REG_WRITE_UINT16(NC_AUXREG_DAT, (CMD_0x10 << 8) | ACT_SER_DOUT);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (CMD_0x70 << 8) | ACT_WAITRB);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_REPEAT << 8) | ACT_CHKSTATUS);
	REG_WRITE_UINT16(NC_AUXREG_DAT, ACT_BREAK);
	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_RPTCNT);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PageCnt-1);

	#if 0==IF_SPARE_AREA_DMA
	if(pu8_SpareBuf)
	    for(u16_Tmp=0; u16_Tmp < pNandDrv->u16_PageSectorCnt; u16_Tmp++)
		    NC_SetCIFD(pu8_SpareBuf + pNandDrv->u16_SectorSpareByteCnt * u16_Tmp,
		           pNandDrv->u16_SectorSpareByteCnt * u16_Tmp,
		           pNandDrv->u16_SectorSpareByteCnt - pNandDrv->u16_ECCCodeByteCnt);
	else
		for(u16_Tmp=0; u16_Tmp < pNandDrv->u16_PageSectorCnt; u16_Tmp++)
		    NC_SetCIFD_Const(0xFF, pNandDrv->u16_SectorSpareByteCnt * u16_Tmp,
		           pNandDrv->u16_SectorSpareByteCnt - pNandDrv->u16_ECCCodeByteCnt);
    #endif

	REG_SET_BITS_UINT16(NC_SIGNAL, BIT_NC_WP_H);
	#if defined(NAND_WP_GPIO) && NAND_WP_GPIO
		REG_SET_BITS_UINT16(ONIF_CONFIG0, R_PIN_RPZ);
	//drvNAND_SetWPGPIO(1);
	#endif

	#if defined(NAND_CE_GPIO) && NAND_CE_GPIO
	drvNAND_SetCEGPIO(0);
	#endif

	REG_WRITE_UINT16(NC_CTRL,
		(BIT_NC_CIFD_ACCESS|BIT_NC_JOB_START|BIT_NC_IF_DIR_W));

	if(NC_WaitComplete(BIT_NC_JOB_END|BIT_MMA_DATA_END, WAIT_WRITE_TIME) == WAIT_WRITE_TIME)
	{
		NANDMSG(nand_printf("Error: NC_WritePages Timeout, ErrCode:%Xh \r\n", UNFD_ST_ERR_W_TIMEOUT));
		NC_ResetFCIE();
		NC_ResetNandFlash();
		NC_Config();
		#if defined(NAND_WP_GPIO) && NAND_WP_GPIO
		REG_CLR_BITS_UINT16(ONIF_CONFIG0, R_PIN_RPZ);
		//drvNAND_SetWPGPIO(0);
		#else
		REG_CLR_BITS_UINT16(NC_SIGNAL, BIT_NC_WP_H);
		#endif

		#if defined(NAND_CE_GPIO) && NAND_CE_GPIO
		drvNAND_SetCEGPIO(1);
		#endif

		NC_UNLOCK_FCIE();
		return UNFD_ST_ERR_W_TIMEOUT; // timeout
	}

	#if defined(NAND_CE_GPIO) && NAND_CE_GPIO
	drvNAND_SetCEGPIO(1);
	#endif

	#if defined(NAND_WP_GPIO) && NAND_WP_GPIO
	REG_CLR_BITS_UINT16(ONIF_CONFIG0, R_PIN_RPZ);
	//drvNAND_SetWPGPIO(0);
	#else
	REG_CLR_BITS_UINT16(NC_SIGNAL, BIT_NC_WP_H);
	#endif

	NC_UNLOCK_FCIE();

	return NC_CheckEWStatus(OPTYPE_WRITE);
}

// can not corss block
U32 NC_ReadPages
(
    U32 u32_PhyRowIdx, U8 *pu8_DataBuf, U8 *pu8_SpareBuf, U32 u32_PageCnt
)
{
	U16 u16_Tmp=0;
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNand_get_DrvContext_address();
	U32 u32_DataDMAAddr;
	U8 u8GiveupRetryCount = 0;
	#if IF_SPARE_AREA_DMA
	U32 u32_SpareDMAAddr;
	#endif

	#if defined(NAND_MIPS_WRITE_BUFFER_PATCH) && NAND_MIPS_WRITE_BUFFER_PATCH
	int i;
	U32 u32Dummy[128];
	U32 *pu32Dummy = u32Dummy;
	#endif

	NANDMSG(printk("%s\n", __FUNCTION__));

	NC_LOCK_FCIE();
	NC_PAD_SWITCH(1);

nandredo:

    // can not corss block
	if((u32_PhyRowIdx & pNandDrv->u16_BlkPageCntMask) + u32_PageCnt >
		pNandDrv->u16_BlkCnt)
	{
		NANDMSG(nand_printf("ERROR: NC_ReadPages, ErrCode:%Xh \r\n", UNFD_ST_ERR_HAL_R_INVALID_PARAM));
		return UNFD_ST_ERR_HAL_R_INVALID_PARAM;
	}

	REG_WRITE_UINT16(NC_MIE_EVENT, BIT_NC_JOB_END|BIT_MMA_DATA_END);

	#if NAND_DMA_RACING_PATCH
    ((U32*)pu8_DataBuf)[pNandDrv->u32_SectorByteCnt/sizeof(U32)-1] = NAND_DMA_RACING_PATTERN;
	  #if IF_SPARE_AREA_DMA
	if(pu8_SpareBuf)
        ((U32*)pu8_SpareBuf)[pNandDrv->u16_SectorSpareByteCnt/sizeof(U32)-1] = NAND_DMA_RACING_PATTERN;
	  #endif
	#endif

	u32_DataDMAAddr = nand_translate_DMA_address_Ex((U32)pu8_DataBuf, pNandDrv->u32_PageByteCnt*u32_PageCnt);

	#if IF_SPARE_AREA_DMA
	if(pu8_SpareBuf)
		u32_SpareDMAAddr = nand_translate_DMA_address_Ex((U32)pu8_SpareBuf, pNandDrv->u16_SpareByteCnt);
	else
		u32_SpareDMAAddr = nand_translate_DMA_address_Ex((U32)pNandDrv->NandDrvPlat_t.pu8_Spare, pNandDrv->u16_SpareByteCnt);
	REG_WRITE_UINT16(NC_SPARE_DMA_ADR0, u32_SpareDMAAddr & 0xFFFF);
	REG_WRITE_UINT16(NC_SPARE_DMA_ADR1, u32_SpareDMAAddr >>16);
	#endif

	  REG_WRITE_UINT16(NC_JOB_BL_CNT, u32_PageCnt << pNandDrv->u8_PageSectorCntBits);
	//REG_WRITE_UINT16(NC_JOB_BL_CNT, 8);

    REG_WRITE_UINT16(NC_SDIO_ADDR0, u32_DataDMAAddr & 0xFFFF);//>>MIU_BUS_WIDTH_BITS));
	REG_WRITE_UINT16(NC_SDIO_ADDR1, u32_DataDMAAddr >> 16);//(MIU_BUS_WIDTH_BITS+16)));
	REG_CLR_BITS_UINT16(NC_MMA_PRI_REG, BIT_NC_DMA_DIR_W);
	REG_SET_BITS_UINT16(NC_PATH_CTL, BIT_MMA_EN);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_ADRSET);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PhyRowIdx & 0xFFFF);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PhyRowIdx >> 16);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (pNandDrv->u8_OpCode_RW_AdrCycle<< 8) | (CMD_0x00));

	if(pNandDrv->u16_Reg48_Spare& BIT_NC_ONE_COL_ADDR) // if a page 512B
	{
		REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_SER_DIN << 8) | ACT_WAITRB);
		REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_BREAK << 8) | ACT_REPEAT);
	}
	else
	{
		REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_WAITRB << 8) | CMD_0x30);
		REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_REPEAT << 8) | ACT_SER_DIN);
		REG_WRITE_UINT16(NC_AUXREG_DAT, ACT_BREAK);
	}
	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_RPTCNT);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PageCnt-1);
	NANDMSG(nand_printf("R Rpt Cnt: %lXh \r\n", u32_PageCnt-1));

	_dma_cache_wback_inv((U32)pu8_DataBuf, u32_PageCnt*pNandDrv->u32_PageByteCnt);
	_dma_cache_wback_inv((U32)pu8_SpareBuf, u32_PageCnt*pNandDrv->u16_SpareByteCnt);

#if 1
	#if defined(NAND_MIPS_WRITE_BUFFER_PATCH) && NAND_MIPS_WRITE_BUFFER_PATCH
	pu32Dummy = (U32*)KSEG02KSEG1((U32)pu32Dummy);
	for(i=0; i<8; i++)
	{
		pu32Dummy[i*16] = 0x01234567*i;
	}
	#else
	Chip_Flush_Memory();
	#endif
#endif

	#if defined(NAND_CE_GPIO) && NAND_CE_GPIO
	drvNAND_SetCEGPIO(0);
	#endif

	REG_WRITE_UINT16(NC_CTRL, (BIT_NC_CIFD_ACCESS|BIT_NC_JOB_START) & ~(BIT_NC_IF_DIR_W));

	if(NC_WaitComplete(BIT_NC_JOB_END|BIT_MMA_DATA_END, WAIT_READ_TIME*u32_PageCnt) ==
	    (WAIT_READ_TIME*u32_PageCnt))
	{
		(nand_printf("Error: NC_ReadPages Timeout, ErrCode:%Xh \r\n", UNFD_ST_ERR_R_TIMEOUT));
		NC_ResetFCIE();
		NC_ResetNandFlash();
		NC_Config();

		if( ++u8GiveupRetryCount > 50 )
        	goto nandredo;
		else
		{
			#if defined(NAND_CE_GPIO) && NAND_CE_GPIO
			drvNAND_SetCEGPIO(1);
			#endif
			return UNFD_ST_ERR_R_TIMEOUT;
		}
	}

	#if defined(NAND_CE_GPIO) && NAND_CE_GPIO
	drvNAND_SetCEGPIO(1);
	#endif
#if 1
	#if defined(NAND_MIPS_READ_BUFFER_PATCH) && NAND_MIPS_READ_BUFFER_PATCH
	Chip_Read_Memory();
	#endif
#endif


#if 0
	int k;
	printk("**********************pu8_DataBuf data print ************** \n");
	for (k = 0; k < 8192; k++) {
		printk("%08x ", pu8_DataBuf[k] );
		if(!(k % 10))
			printk("\n");
		}
	printk("**********************pu8_DataBuf data print ************** \n");
#endif

#if 0
		// Check ECC
	REG_READ_UINT16(NC_ECC_STAT2, u16_Tmp);
	if(u16_Tmp & BIT_NC_ECC_FLGA_CORRECT)
	{
		(nand_printf("Reg53:%04Xh\n", u16_Tmp));
		REG_READ_UINT16(NC_ECC_STAT0, u16_Tmp);
		(nand_printf("Error: NC_ReadPages ECC Correction, @Sector %d\nReg51:%04Xh\n", (u16_Tmp>>8)&0x3F, u16_Tmp));
		REG_READ_UINT16(NC_ECC_STAT1, u16_Tmp);
		NANDMSG(nand_printf("Reg52:%04Xh\n", u16_Tmp));

	}

#endif

	// Check ECC
	REG_READ_UINT16(NC_ECC_STAT0, u16_Tmp);
	if(u16_Tmp & BIT_NC_ECC_FAIL)
	{
		(nand_printf("Error: NC_ReadPages ECC Fail, @Sector %d\nReg51:%04Xh\n", (u16_Tmp>>8)&0x3F, u16_Tmp));
		REG_READ_UINT16(NC_ECC_STAT1, u16_Tmp);
		NANDMSG(nand_printf("Reg52:%04Xh\n", u16_Tmp));
		REG_READ_UINT16(NC_ECC_STAT2, u16_Tmp);
		(nand_printf("Reg53:%04Xh\n", u16_Tmp));

		NC_UNLOCK_FCIE();

		return UNFD_ST_ERR_ECC_FAIL;
	}


	// patch MIU
    #if NAND_DMA_RACING_PATCH
    for(u16_Tmp=0; u16_Tmp<NAND_DMA_PATCH_WAIT_TIME; u16_Tmp++)
    {
		if(((U32*)pu8_DataBuf)[pNandDrv->u32_SectorByteCnt/sizeof(U32)-1] != NAND_DMA_RACING_PATTERN)
		  #if IF_SPARE_AREA_DMA
        if(pu8_SpareBuf && ((U32*)pu8_SpareBuf)[pNandDrv->u16_SectorSpareByteCnt/sizeof(U32)-1] != NAND_DMA_RACING_PATTERN)
		  #endif
			break;
		nand_hw_timer_delay(HW_TIMER_DELAY_1us);
    }
	if(NAND_DMA_PATCH_WAIT_TIME == u16_Tmp)
	{
		(nand_printf("ERROR: NC_ReadPages MIU Patch Timeout, ErrCode:%Xh \r\n", UNFD_ST_ERR_MIU_RPATCH_TIMEOUT));

		NC_UNLOCK_FCIE();

		return UNFD_ST_ERR_MIU_RPATCH_TIMEOUT;
	}
	#endif


	#if 0==IF_SPARE_AREA_DMA
	if(pu8_SpareBuf)
		NC_GetCIFD(pu8_SpareBuf, 0, pNandDrv->u16_SpareByteCnt);
	#endif

	NC_UNLOCK_FCIE();

	return UNFD_ST_SUCCESS;
}

/*--------------------------------------------------
  get ECC corrected bits count

  return: 0xFFFFFFFF -> ECC uncorrectable error,
  other: max ECC corrected bits (within the readed sectors).
  --------------------------------------------------*/
U32  NC_GetECCBits(void)
{
	U16 u16_Tmp;

	REG_READ_UINT16(NC_ECC_STAT0, u16_Tmp);
	if(u16_Tmp & BIT_NC_ECC_FAIL)
		return (U32)(0-1);

	return (u16_Tmp & BIT_NC_ECC_MAX_BITS_MASK) >> 1;
}

U32 NC_Read_RandomIn
(
    U32 u32_PhyRow, U32 u32Col, U8 *pu8DataBuf, U32 u32DataByteCnt
)
{
	U32 u32_Tmp;
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNand_get_DrvContext_address();

	NC_LOCK_FCIE();

	NC_PAD_SWITCH(1);
	REG_WRITE_UINT16(NC_MIE_EVENT, BIT_NC_JOB_END|BIT_MMA_DATA_END);
	#if IF_SPARE_AREA_DMA
    pNandDrv->u16_Reg48_Spare &= ~BIT_NC_SPARE_DEST_MIU;
	#endif
	pNandDrv->u16_Reg48_Spare |= BIT_NC_RANDOM_RW_OP_EN;
	REG_WRITE_UINT16(NC_SPARE, pNandDrv->u16_Reg48_Spare);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_ADRSET);
	if(pNandDrv->u16_Reg48_Spare & BIT_NC_ONE_COL_ADDR)
		REG_WRITE_UINT16(NC_AUXREG_DAT, u32Col<<8);
	else
		REG_WRITE_UINT16(NC_AUXREG_DAT, u32Col);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PhyRow & 0xFFFF);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PhyRow >> 16);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (pNandDrv->u8_OpCode_RW_AdrCycle << 8) | (CMD_0x00));
	if(pNandDrv->u16_Reg48_Spare & BIT_NC_ONE_COL_ADDR)
	{
		REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_RAN_DIN << 8) | ACT_WAITRB);
		REG_WRITE_UINT16(NC_AUXREG_DAT, ACT_BREAK);
	}
	else
	{
		REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_WAITRB << 8) | CMD_0x30);
		REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_BREAK << 8) | ACT_RAN_DIN);
	}

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_RAN_CNT);
	u32_Tmp = u32DataByteCnt + (u32DataByteCnt & 0x01); // byet-count needs to be word-alignment
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_Tmp);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0); // offset;

	#if defined(NAND_CE_GPIO) && NAND_CE_GPIO
	drvNAND_SetCEGPIO(0);
	#endif

	REG_WRITE_UINT16(NC_CTRL, BIT_NC_CIFD_ACCESS|BIT_NC_JOB_START);
	if(NC_WaitComplete(BIT_NC_JOB_END, WAIT_READ_TIME) == WAIT_READ_TIME)
	{
		NANDMSG(nand_printf("Err: drvNand_ReadRandomData_Ex - timeout \r\n"));
		NC_ResetFCIE();
		NC_ResetNandFlash();
		#if IF_SPARE_AREA_DMA
        pNandDrv->u16_Reg48_Spare |= BIT_NC_SPARE_DEST_MIU;
	    #endif
		pNandDrv->u16_Reg48_Spare &= ~BIT_NC_RANDOM_RW_OP_EN;
		NC_Config();

		#if defined(NAND_CE_GPIO) && NAND_CE_GPIO
		drvNAND_SetCEGPIO(1);
		#endif

		NC_UNLOCK_FCIE();

		return UNFD_ST_ERR_R_TIMEOUT; // timeout
	}

	/* get data from CIFD */
	NC_GetCIFD(pu8DataBuf, 0, u32DataByteCnt);
	#if IF_SPARE_AREA_DMA
    pNandDrv->u16_Reg48_Spare |= BIT_NC_SPARE_DEST_MIU;
	#endif
	pNandDrv->u16_Reg48_Spare &= ~BIT_NC_RANDOM_RW_OP_EN;
	REG_WRITE_UINT16(NC_SPARE, pNandDrv->u16_Reg48_Spare);

	#if defined(NAND_CE_GPIO) && NAND_CE_GPIO
	drvNAND_SetCEGPIO(1);
	#endif

	NC_UNLOCK_FCIE();

	return UNFD_ST_SUCCESS;
}

U32 NC_Write_RandomOut
(
    U32 u32_PhyRow, U32 u32_Col, U8 *pu8_DataBuf, U32 u32_DataByteCnt
)
{
	U32 u32_Tmp;
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNand_get_DrvContext_address();

	NC_LOCK_FCIE();

	NC_PAD_SWITCH(1);
	REG_WRITE_UINT16(NC_MIE_EVENT, BIT_NC_JOB_END|BIT_MMA_DATA_END);
	#if IF_SPARE_AREA_DMA
    pNandDrv->u16_Reg48_Spare &= ~BIT_NC_SPARE_DEST_MIU;
	#endif
	pNandDrv->u16_Reg48_Spare |= BIT_NC_RANDOM_RW_OP_EN;
	REG_WRITE_UINT16(NC_SPARE, pNandDrv->u16_Reg48_Spare);

    // set data into CIFD
    NC_SetCIFD(pu8_DataBuf, 0, u32_DataByteCnt);
	if(u32_DataByteCnt & 1)
	{
		U8 au8_Tmp[1];
		au8_Tmp[0] = 0xFF; // pad a 0xFF
		NC_SetCIFD(au8_Tmp, u32_DataByteCnt, 1);
	}
	//while(1)  nand_reset_WatchDog();

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_ADRSET);
	if(pNandDrv->u16_Reg48_Spare & BIT_NC_ONE_COL_ADDR)
		REG_WRITE_UINT16(NC_AUXREG_DAT, u32_Col<<8);
	else
		REG_WRITE_UINT16(NC_AUXREG_DAT, u32_Col);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PhyRow & 0xFFFF);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PhyRow >> 16);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (pNandDrv->u8_OpCode_RW_AdrCycle << 8) | (CMD_0x80));
	REG_WRITE_UINT16(NC_AUXREG_DAT, (CMD_0x10 << 8) | ACT_RAN_DOUT);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (CMD_0x70 << 8)| ACT_WAITRB);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_BREAK << 8)| ACT_CHKSTATUS);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_RAN_CNT);
	u32_Tmp = u32_DataByteCnt + (u32_DataByteCnt & 0x01); // byet-count needs to be word-alignment
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_Tmp);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0); // offset;

	#if defined(NAND_CE_GPIO) && NAND_CE_GPIO
	drvNAND_SetCEGPIO(0);
	#endif

	REG_WRITE_UINT16(NC_CTRL, BIT_NC_CIFD_ACCESS|BIT_NC_JOB_START);
	if(NC_WaitComplete(BIT_NC_JOB_END, WAIT_WRITE_TIME) == WAIT_WRITE_TIME)
	{
		NANDMSG(nand_printf("Error: NC_Write_RandomOut Timeout, ErrCode:%Xh \r\n", UNFD_ST_ERR_W_TIMEOUT));
		#if 0==IF_IP_VERIFY
		NC_ResetFCIE();
		NC_ResetNandFlash();
		#if IF_SPARE_AREA_DMA
        pNandDrv->u16_Reg48_Spare |= BIT_NC_SPARE_DEST_MIU;
	    #endif
		pNandDrv->u16_Reg48_Spare &= ~BIT_NC_RANDOM_RW_OP_EN;
		NC_Config();
		#endif

		#if defined(NAND_CE_GPIO) && NAND_CE_GPIO
		drvNAND_SetCEGPIO(1);
		#endif

		NC_UNLOCK_FCIE();

		return UNFD_ST_ERR_W_TIMEOUT; // timeout
	}

	#if defined(NAND_CE_GPIO) && NAND_CE_GPIO
	drvNAND_SetCEGPIO(1);
	#endif

    #if IF_SPARE_AREA_DMA
    pNandDrv->u16_Reg48_Spare |= BIT_NC_SPARE_DEST_MIU;
	#endif
	pNandDrv->u16_Reg48_Spare &= ~BIT_NC_RANDOM_RW_OP_EN;
	REG_WRITE_UINT16(NC_SPARE, pNandDrv->u16_Reg48_Spare);

	NC_UNLOCK_FCIE();

	return NC_CheckEWStatus(OPTYPE_WRITE);
}


//  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
const U8 u8FSTYPE[256] =
{	0,19, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 1
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 2
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 3
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 4
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 5
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 6
	0,18, 0, 6, 0, 8,10, 0, 0,12, 0, 0, 0, 0, 0, 0, // 7
	0, 0, 0, 0, 0, 0, 0, 0 ,0, 0, 0, 0, 0, 0, 0, 0, // 8
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 9
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // A
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // B
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // C
	0,13, 0,16, 0,17, 3, 0, 0, 0,15, 0,14, 0, 0, 0, // D
	0, 0, 0, 2, 0, 2, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, // E
   20,13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // F
};

void drvNAND_CHECK_FLASH_TYPE(NAND_DRIVER *pNandDrv)
{
	if( (pNandDrv->au8_ID[0] != VENDOR_SAMSUMG) &&
		(pNandDrv->au8_ID[0] != VENDOR_TOSHIBA) &&
		(pNandDrv->au8_ID[0] != VENDOR_RENESAS) &&
		(pNandDrv->au8_ID[0] != VENDOR_HYNIX)  &&
		(pNandDrv->au8_ID[0] != VENDOR_ST) )
	{
		pNandDrv->u16_BlkCnt = 0;
		pNandDrv->u16_BlkPageCnt = 0;
		pNandDrv->u32_PageByteCnt = 0;
		pNandDrv->u32_SectorByteCnt = 0;
		pNandDrv->u16_SpareByteCnt = 0;

		return; // unsupported flash maker
	}

	switch(u8FSTYPE[pNandDrv->au8_ID[1]])
	{
		case 0:
			pNandDrv->u16_BlkCnt = 0;
			pNandDrv->u16_BlkPageCnt = 0;
			pNandDrv->u32_PageByteCnt = 0;
			pNandDrv->u32_SectorByteCnt = 0;
			pNandDrv->u16_SpareByteCnt = 0;
			return;
		case 2:
			pNandDrv->u16_BlkCnt = 512;
			pNandDrv->u16_BlkPageCnt = 16;
			pNandDrv->u32_PageByteCnt = 512;
			pNandDrv->u32_SectorByteCnt = 512;
			pNandDrv->u16_SpareByteCnt = 16;
			return;
		case 4:
			pNandDrv->u16_BlkCnt = 1024;
			pNandDrv->u16_BlkPageCnt = 16;
			pNandDrv->u32_PageByteCnt = 512;
			pNandDrv->u32_SectorByteCnt = 512;
			pNandDrv->u16_SpareByteCnt = 16;
			pNandDrv->u8_OpCode_RW_AdrCycle = ADR_C3TFS0;
			pNandDrv->u8_OpCode_Erase_AdrCycle = ADR_C2TRS0;
			return;
		case 6:
			pNandDrv->u16_BlkCnt = 1024;
			pNandDrv->u16_BlkPageCnt = 32;
			pNandDrv->u32_PageByteCnt = 512;
			pNandDrv->u32_SectorByteCnt = 512;
			pNandDrv->u16_SpareByteCnt = 16;
			pNandDrv->u8_OpCode_RW_AdrCycle = ADR_C3TFS0;
			pNandDrv->u8_OpCode_Erase_AdrCycle = ADR_C2TRS0;
			return;
		case 7:
			//_fsinfo.eFlashConfig |= FLASH_WP;
		case 8:
			pNandDrv->u16_BlkCnt = 2048;
			pNandDrv->u16_BlkPageCnt = 32;
			pNandDrv->u32_PageByteCnt = 512;
			pNandDrv->u32_SectorByteCnt = 512;
			pNandDrv->u16_SpareByteCnt = 16;
			pNandDrv->u8_OpCode_RW_AdrCycle = ADR_C3TFS0;
			pNandDrv->u8_OpCode_Erase_AdrCycle = ADR_C2TRS0;
			return;
		case 10:
			pNandDrv->u16_BlkCnt = 4096;
			pNandDrv->u16_BlkPageCnt = 32;
			pNandDrv->u32_PageByteCnt = 512;
			pNandDrv->u32_SectorByteCnt = 512;
			pNandDrv->u16_SpareByteCnt = 16;
			pNandDrv->u8_OpCode_RW_AdrCycle = ADR_C4TFS0;
			pNandDrv->u8_OpCode_Erase_AdrCycle = ADR_C3TRS0;
			return;
		case 12:
			pNandDrv->u16_BlkCnt = 8192;
			pNandDrv->u16_BlkPageCnt = 32;
			pNandDrv->u32_PageByteCnt = 512;
			pNandDrv->u32_SectorByteCnt = 512;
			pNandDrv->u16_SpareByteCnt = 16;
			pNandDrv->u8_OpCode_RW_AdrCycle = ADR_C4TFS0;
			pNandDrv->u8_OpCode_Erase_AdrCycle = ADR_C3TRS0;
			return;
		case 13:
			pNandDrv->u16_BlkCnt = 1024;
			pNandDrv->u16_BlkPageCnt = 64;
			pNandDrv->u32_PageByteCnt = 2048;
			pNandDrv->u32_SectorByteCnt = 512;
			pNandDrv->u16_SpareByteCnt = 64;
			pNandDrv->u8_OpCode_RW_AdrCycle = ADR_C4TFS0;
			pNandDrv->u8_OpCode_Erase_AdrCycle = ADR_C2TRS0;
			return;
		case 14:
			if(pNandDrv->au8_ID[0]==VENDOR_HYNIX)
			{
				pNandDrv->u16_BlkCnt = 4096;
				pNandDrv->u16_BlkPageCnt = 128;
				pNandDrv->u32_PageByteCnt = 2048;
				pNandDrv->u32_SectorByteCnt = 512;
				pNandDrv->u16_SpareByteCnt = 64;
			}
			else
			{
				pNandDrv->u16_BlkCnt = 4096;
				pNandDrv->u16_BlkPageCnt = 64;
				pNandDrv->u32_PageByteCnt = 2048;
				pNandDrv->u32_SectorByteCnt = 512;
				pNandDrv->u16_SpareByteCnt = 64;
			}
			pNandDrv->u8_OpCode_RW_AdrCycle = ADR_C5TFS0;
			pNandDrv->u8_OpCode_Erase_AdrCycle = ADR_C3TRS0;
			return;
		case 15:
			pNandDrv->u16_BlkCnt = 2048;
			pNandDrv->u16_BlkPageCnt = 64;
			pNandDrv->u32_PageByteCnt = 2048;
			pNandDrv->u32_SectorByteCnt = 512;
			pNandDrv->u16_SpareByteCnt = 64;
			pNandDrv->u8_OpCode_RW_AdrCycle = ADR_C5TFS0;
			pNandDrv->u8_OpCode_Erase_AdrCycle = ADR_C3TRS0;
			return;
		case 16:
			 if((pNandDrv->au8_ID[0]==VENDOR_HYNIX))
			{
				pNandDrv->u16_BlkCnt = 4096;
				pNandDrv->u16_BlkPageCnt = 128;
				pNandDrv->u32_PageByteCnt = 2048;
				pNandDrv->u32_SectorByteCnt = 512;
				pNandDrv->u16_SpareByteCnt = 64;
			}
			else if(pNandDrv->au8_ID[0] != VENDOR_ST)
			{
				pNandDrv->u16_BlkCnt = 2048;
				pNandDrv->u16_BlkPageCnt = 64;
				pNandDrv->u32_PageByteCnt = 2048;
				pNandDrv->u32_SectorByteCnt = 512;
				pNandDrv->u16_SpareByteCnt = 64;
			}
			else
			{
				pNandDrv->u16_BlkCnt = 8192;
				pNandDrv->u16_BlkPageCnt = 64;
				pNandDrv->u32_PageByteCnt = 2048;
				pNandDrv->u32_SectorByteCnt = 512;
				pNandDrv->u16_SpareByteCnt = 64;
			}
			pNandDrv->u8_OpCode_RW_AdrCycle = ADR_C5TFS0;
			pNandDrv->u8_OpCode_Erase_AdrCycle = ADR_C3TRS0;
			return;
		case 17:
			if(pNandDrv->au8_ID[0] != VENDOR_SAMSUMG)
			{
				pNandDrv->u16_BlkCnt = 8192;
				pNandDrv->u16_BlkPageCnt = 128;
				pNandDrv->u32_PageByteCnt = 2048;
				pNandDrv->u32_SectorByteCnt = 512;
				pNandDrv->u16_SpareByteCnt = 64;
			}
			else
			{
				pNandDrv->u16_BlkCnt = 2076;
				pNandDrv->u16_BlkPageCnt = 128;
				pNandDrv->u32_PageByteCnt = 8192;
				pNandDrv->u32_SectorByteCnt = 1024;
				pNandDrv->u16_SpareByteCnt = 432;
			}
			pNandDrv->u8_OpCode_RW_AdrCycle = ADR_C5TFS0;
			pNandDrv->u8_OpCode_Erase_AdrCycle = ADR_C3TRS0;
			return;
		case 18:
			pNandDrv->u16_BlkCnt = 16384;
			pNandDrv->u16_BlkPageCnt = 32;
			pNandDrv->u32_PageByteCnt = 512;
			pNandDrv->u32_SectorByteCnt = 512;
			pNandDrv->u16_SpareByteCnt = 16;
			pNandDrv->u8_OpCode_RW_AdrCycle = ADR_C4TFS0;
			pNandDrv->u8_OpCode_Erase_AdrCycle = ADR_C3TRS0;
			return;
		case 20:
			pNandDrv->u16_BlkCnt = 512;
			pNandDrv->u16_BlkPageCnt = 64;
			pNandDrv->u32_PageByteCnt = 2048;
			pNandDrv->u32_SectorByteCnt = 512;
			pNandDrv->u16_SpareByteCnt = 64;
			pNandDrv->u8_OpCode_RW_AdrCycle = ADR_C4TFS0;
			pNandDrv->u8_OpCode_Erase_AdrCycle = ADR_C2TRS0;
			return;
		default:
			pNandDrv->u16_BlkCnt = 0;
			pNandDrv->u16_BlkPageCnt = 0;
			pNandDrv->u32_PageByteCnt = 0;
			pNandDrv->u32_SectorByteCnt = 0;
			pNandDrv->u16_SpareByteCnt = 0;
			return;
	}
}

U32 NC_ReadID(void)
{
	volatile U16 u16_Reg, u16_i;
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNand_get_DrvContext_address();

	NC_LOCK_FCIE();

	NC_PAD_SWITCH(1);

  REG_WRITE_UINT16(NC_SIGNAL, 0x40 );
	REG_WRITE_UINT16(NC_MIE_EVENT, BIT_NC_JOB_END);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_ADRSET);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ADR_C2T1S0 << 8) | CMD_0x90);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_BREAK << 8) | ACT_RAN_DIN);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_RAN_CNT);
	REG_WRITE_UINT16(NC_AUXREG_DAT, NAND_ID_BYTE_CNT);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0); /*offset 0*/

	#if defined(NAND_CE_GPIO) && NAND_CE_GPIO
	drvNAND_SetCEGPIO(0);
	#endif

    REG_WRITE_UINT16(NC_CTRL, BIT_NC_CIFD_ACCESS | BIT_NC_JOB_START);

	if(NC_WaitComplete(BIT_NC_JOB_END, DELAY_500ms_in_us) == DELAY_500ms_in_us)
	{
		(nand_printf(KERN_ERR "Error! Read ID timeout! \r\n"));

		REG_READ_UINT16(NC_MIE_EVENT, u16_Reg);
		(nand_printf("NC_MIE_EVENT: %Xh \r\n", u16_Reg));
		REG_READ_UINT16(NC_CTRL, u16_Reg);
		(nand_printf("NC_CTRL: %Xh \r\n", u16_Reg));

		#if defined(NAND_CE_GPIO) && NAND_CE_GPIO
		drvNAND_SetCEGPIO(1);
		#endif

		NC_UNLOCK_FCIE();

		return UNFD_ST_ERR_R_TIMEOUT; // timeout
	}

	NC_GetCIFD(pNandDrv->au8_ID, 0, NAND_ID_BYTE_CNT);

	for(u16_i=0; u16_i<NAND_ID_BYTE_CNT; u16_i++)
	{
		if(0!=u16_i)
		{
			if(pNandDrv->au8_ID[u16_i] != pNandDrv->au8_ID[0])
			{
				NANDMSG(nand_printf("%Xh ", pNandDrv->au8_ID[u16_i]));
			}
			else
			{
				pNandDrv->u8_IDByteCnt = u16_i;
				NANDMSG(nand_printf("\r\n  total %u bytes ID.\r\n", pNandDrv->u8_IDByteCnt));
				break;
			}
		}
		else
		{
			NANDMSG(nand_printf("ID: %Xh ", pNandDrv->au8_ID[u16_i]));
		}
	}

	#if defined(NAND_CE_GPIO) && NAND_CE_GPIO
	drvNAND_SetCEGPIO(1);
	#endif

	NC_UNLOCK_FCIE();

	return UNFD_ST_SUCCESS; // ok
}

U32 NC_EraseBlk(U32 u32_PhyRowIdx)
{
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNand_get_DrvContext_address();

	NC_LOCK_FCIE();
	NC_PAD_SWITCH(1);

	REG_W1C_BITS_UINT16(NC_MIE_EVENT, BIT_NC_JOB_END);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_ADRSET);
	REG_WRITE_UINT16(NC_AUXREG_DAT, 0);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PhyRowIdx & 0xFFFF);
	REG_WRITE_UINT16(NC_AUXREG_DAT, u32_PhyRowIdx >> 16);

    REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
	REG_WRITE_UINT16(NC_AUXREG_DAT,	(pNandDrv->u8_OpCode_Erase_AdrCycle << 8) | CMD_0x60);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_WAITRB << 8) | CMD_0xD0);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_CHKSTATUS << 8) | CMD_0x70);
	REG_WRITE_UINT16(NC_AUXREG_DAT, ACT_BREAK);

	REG_SET_BITS_UINT16(NC_SIGNAL, BIT_NC_WP_H);
	#if defined(NAND_WP_GPIO) && NAND_WP_GPIO
	REG_SET_BITS_UINT16(ONIF_CONFIG0, R_PIN_RPZ);
	//drvNAND_SetWPGPIO(1);
	#endif

	#if defined(NAND_CE_GPIO) && NAND_CE_GPIO
	drvNAND_SetCEGPIO(0);
	#endif

    REG_WRITE_UINT16(NC_CTRL, BIT_NC_JOB_START);
	if(NC_WaitComplete(BIT_NC_JOB_END, WAIT_ERASE_TIME) == WAIT_ERASE_TIME)
	{
		NANDMSG(nand_printf("Error: NC_EraseBlk Timeout, ErrCode:%Xh \r\n", UNFD_ST_ERR_E_TIMEOUT));
		NC_ResetFCIE();
		NC_ResetNandFlash();
		NC_Config();
		#if defined(NAND_WP_GPIO) && NAND_WP_GPIO
		REG_CLR_BITS_UINT16(ONIF_CONFIG0, R_PIN_RPZ);
		//drvNAND_SetWPGPIO(0);
		#else
		REG_CLR_BITS_UINT16(NC_SIGNAL, BIT_NC_WP_H);
		#endif

		#if defined(NAND_CE_GPIO) && NAND_CE_GPIO
		drvNAND_SetCEGPIO(1);
		#endif

		NC_UNLOCK_FCIE();

		return UNFD_ST_ERR_E_TIMEOUT;
	}

	#if defined(NAND_WP_GPIO) && NAND_WP_GPIO
	REG_CLR_BITS_UINT16(ONIF_CONFIG0, R_PIN_RPZ);
	//drvNAND_SetWPGPIO(0);
	#else
	REG_CLR_BITS_UINT16(NC_SIGNAL, BIT_NC_WP_H);
	#endif

	#if defined(NAND_CE_GPIO) && NAND_CE_GPIO
	drvNAND_SetCEGPIO(1);
	#endif

	NC_UNLOCK_FCIE();

	return NC_CheckEWStatus(OPTYPE_ERASE);
}

static U32 NC_CheckEWStatus(U8 u8_OpType)
{
	volatile U16 u16_Tmp;
	U32 u32_ErrCode = 0;

	REG_READ_UINT16(NC_ST_READ, u16_Tmp);

	if((u16_Tmp & BIT_ST_READ_FAIL) == 1) // if fail
	{
		if(OPTYPE_ERASE == u8_OpType)
			u32_ErrCode = UNFD_ST_ERR_E_FAIL;
		else if(OPTYPE_WRITE == u8_OpType)
			u32_ErrCode = UNFD_ST_ERR_W_FAIL;

		NANDMSG(nand_printf("ERROR: NC_CheckEWStatus Fail, Nand St:%Xh, ErrCode:%Xh \r\n",
			REG(NC_ST_READ), (unsigned int)u32_ErrCode));
		return u32_ErrCode;
	}
	else if((u16_Tmp & BIT_ST_READ_BUSYn) == 0) // if busy
	{
		if(OPTYPE_ERASE == u8_OpType)
			u32_ErrCode = UNFD_ST_ERR_E_BUSY;
		else if(OPTYPE_WRITE == u8_OpType)
			u32_ErrCode = UNFD_ST_ERR_W_BUSY;

		NANDMSG(nand_printf("ERROR: NC_CheckEWStatus Busy, Nand St:%Xh, ErrCode:%Xh \r\n",
			REG(NC_ST_READ), (unsigned int)u32_ErrCode));
		return u32_ErrCode;
	}
	else if((u16_Tmp & BIT_ST_READ_PROTECTn) == 0) // if protected
	{
		if(OPTYPE_ERASE == u8_OpType)
			u32_ErrCode = UNFD_ST_ERR_E_PROTECTED;
		else if(OPTYPE_WRITE == u8_OpType)
			u32_ErrCode = UNFD_ST_ERR_W_PROTECTED;

		NANDMSG(nand_printf("ERROR: NC_CheckEWStatus Protected, Nand St:%Xh, ErrCode:%Xh \r\n",
			REG(NC_ST_READ), (unsigned int)u32_ErrCode));
		return u32_ErrCode;
	}

	return UNFD_ST_SUCCESS;
}

U32 NC_ResetNandFlash(void)
{
	NC_PAD_SWITCH(1);
	REG_W1C_BITS_UINT16(NC_MIE_EVENT, BIT_NC_JOB_END);

        REG_WRITE_UINT16(NC_SIGNAL, 0x40 );
        
	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_WAITRB << 8) | CMD_0xFF);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_BREAK<<8)|ACT_BREAK);

	#if defined(NAND_CE_GPIO) && NAND_CE_GPIO
	drvNAND_SetCEGPIO(0);
	#endif

	REG_WRITE_UINT16(NC_CTRL, BIT_NC_JOB_START);

	if(NC_WaitComplete(BIT_NC_JOB_END, WAIT_RESET_TIME) == WAIT_RESET_TIME)
	{
		NANDMSG(nand_printf("ERROR: NC_ResetNandFlash, ErrCode:%Xh \r\n", UNFD_ST_ERR_RST_TIMEOUT));
		return UNFD_ST_ERR_RST_TIMEOUT;
	}

	#if defined(NAND_CE_GPIO) && NAND_CE_GPIO
	drvNAND_SetCEGPIO(1);
	#endif
	
	return UNFD_ST_SUCCESS;
}

U32 NC_WaitComplete(U16 u16_WaitEvent, U32 u32_MicroSec)
{
	volatile U32 u32_Count;
    volatile U16 u16_Reg;

	for(u32_Count=0; u32_Count < u32_MicroSec; u32_Count++)
	{
		REG_READ_UINT16(NC_MIE_EVENT, u16_Reg);
	    if((u16_Reg & u16_WaitEvent) == u16_WaitEvent)  break;

		nand_hw_timer_delay(HW_TIMER_DELAY_1us);
		nand_reset_WatchDog();
	}

	if(u32_Count < u32_MicroSec)
		REG_W1C_BITS_UINT16(NC_MIE_EVENT, u16_WaitEvent); /*clear events*/

	return u32_Count;
}

void NC_SetCIFD_Const(U8 u8_Data, U32 u32_CIFDPos, U32 u32_ByteCnt)
{
    U32 u32_i;
	volatile U16 u16_Tmp;

	if(u32_CIFDPos & 1)
	{
		REG_READ_UINT16(NC_CIFD_ADDR(u32_CIFDPos>>1), u16_Tmp);
		u16_Tmp &= 0x00FF;
		u16_Tmp += u8_Data << 8;
		REG_WRITE_UINT16(NC_CIFD_ADDR(u32_CIFDPos>>1), u16_Tmp);
		u32_CIFDPos += 1;
		u32_ByteCnt -= 1;
	}

	for(u32_i=0; u32_i<u32_ByteCnt>>1; u32_i++)
	{
		u16_Tmp = u8_Data + (u8_Data << 8);
		REG_WRITE_UINT16(NC_CIFD_ADDR(u32_i+(u32_CIFDPos>>1)), u16_Tmp);
	}

	if(u32_ByteCnt - (u32_i<<1))
	{
		REG_READ_UINT16(NC_CIFD_ADDR(u32_i+(u32_CIFDPos>>1)), u16_Tmp);
		u16_Tmp = (u16_Tmp & 0xFF00) + u8_Data;
		REG_WRITE_UINT16(NC_CIFD_ADDR(u32_i+(u32_CIFDPos>>1)), u16_Tmp);
	}
}

void NC_SetCIFD(U8 *pu8_Buf, U32 u32_CIFDPos, U32 u32_ByteCnt)
{
    U32 u32_i, u32_BufPos;
	volatile U16 u16_Tmp;

	if(u32_CIFDPos & 1)
	{
		REG_READ_UINT16(NC_CIFD_ADDR(u32_CIFDPos>>1), u16_Tmp);
		u16_Tmp &= 0x00FF;
		u16_Tmp += pu8_Buf[0] << 8;
		REG_WRITE_UINT16(NC_CIFD_ADDR(u32_CIFDPos>>1), u16_Tmp);
		u32_CIFDPos += 1;
		u32_ByteCnt -= 1;
		u32_BufPos = 1;
	}
	else  u32_BufPos = 0;

	for(u32_i=0; u32_i<u32_ByteCnt>>1; u32_i++)
	{
		u16_Tmp = pu8_Buf[(u32_i<<1)+u32_BufPos] +
			     (pu8_Buf[(u32_i<<1)+u32_BufPos+1] << 8);
		REG_WRITE_UINT16(NC_CIFD_ADDR(u32_i+(u32_CIFDPos>>1)), u16_Tmp);
	}

	if(u32_ByteCnt - (u32_i<<1))
	{
		REG_READ_UINT16(NC_CIFD_ADDR(u32_i+(u32_CIFDPos>>1)), u16_Tmp);
		u16_Tmp = (u16_Tmp & 0xFF00) + pu8_Buf[(u32_i<<1)+u32_BufPos];
		REG_WRITE_UINT16(NC_CIFD_ADDR(u32_i+(u32_CIFDPos>>1)), u16_Tmp);
	}
}

void NC_GetCIFD(U8 *pu8_Buf, U32 u32_CIFDPos, U32 u32_ByteCnt)
{
    U32 u32_i, u32_BufPos;
	U16 u16_Tmp;

	if(u32_CIFDPos & 1)
	{
		REG_READ_UINT16(NC_CIFD_ADDR(u32_CIFDPos>>1), u16_Tmp);
		pu8_Buf[0] = (U8)(u16_Tmp >> 8);
		u32_CIFDPos += 1;
		u32_ByteCnt -= 1;
		u32_BufPos = 1;
	}
	else
		u32_BufPos = 0;

	for(u32_i=0; u32_i<u32_ByteCnt>>1; u32_i++)
	{
		REG_READ_UINT16(NC_CIFD_ADDR(u32_i+(u32_CIFDPos>>1)), u16_Tmp);
		pu8_Buf[(u32_i<<1)+u32_BufPos] = (U8)u16_Tmp;
		pu8_Buf[(u32_i<<1)+u32_BufPos+1] = (U8)(u16_Tmp>>8);
	}

	if(u32_ByteCnt - (u32_i<<1))
	{
		REG_READ_UINT16(NC_CIFD_ADDR(u32_i+(u32_CIFDPos>>1)), u16_Tmp);
		pu8_Buf[(u32_i<<1)+u32_BufPos] = (U8)u16_Tmp;
	}
}

#if defined(NAND_CE_GPIO) && NAND_CE_GPIO
void drvNAND_SetCEGPIO(U8 u8Level)
{
	U16 u16Tmp;

	HAL_READ_UINT16(reg_gpio_pm_oen, u16Tmp);
	u16Tmp &= (~BIT0);
	
	if( u8Level == 0 )
	{
		u16Tmp &= (~BIT1);
	}
	else
	{
		u16Tmp |= BIT1;
	}
	HAL_WRITE_UINT16(reg_gpio_pm_oen, u16Tmp);
}
#endif

#if defined(NAND_WP_GPIO) && NAND_WP_GPIO
void drvNAND_SetWPGPIO(U8 u8Level)
{
	U16 u16Tmp;

	HAL_READ_UINT16(reg_ts0_gpio_out01, u16Tmp);
	u16Tmp &= (~BIT8);
	
	if( u8Level == 0 )
	{
		u16Tmp &= (~BIT9);
	}
	else
	{
		u16Tmp |= BIT9;
	}
	HAL_WRITE_UINT16(reg_ts0_gpio_out01, u16Tmp);
}
#endif


#endif // NC_SEL_FCIE3
