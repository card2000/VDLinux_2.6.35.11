//=======================================================================
//  MStar Semiconductor - Unified Nand Flash Driver
//
//  drvNand.c - Storage Team, 2009/08/20 
//
//=======================================================================

#include "../include/drvNand.h"

//========================================================
// variable instances
//========================================================
static NAND_DRIVER sg_NandDrv_t;

//========================================================
// UNFD exposed function 
//========================================================

//--------
// for AP
//--------
U32 drvNAND_Init(void);

//------------------------
// for UNFD's other files
//------------------------
U32 drvNand_get_DrvContext_address(void);
U8  drvNand_CountBits(U32 u32_x);
U8  drvNAND_ChkSum(U8 *pBuf, U16 u16ByteCnt);

//========================================================
// function definitions
//======================================================== 
#define nand_get_DrvContext_address()	(U32)&sg_NandDrv_t;	// if not ROM code, save CPU JMP/RET time

U32 drvNand_get_DrvContext_address(void) // for UNFD's other files use
{
	return (U32)&sg_NandDrv_t;  // if not ROM code
}

// NAND Info
#define NCTEST_ID_BYTE_CNT		5
//static U8 sgau8_ID_HardCode[NCTEST_ID_BYTE_CNT] = {0xAD, 0xD5, 0x14, 0xB6, 0x44};

#define NAND_BLK_CNT			0x1000
#define NAND_BLK_PAGE_CNT		0x80
#define NAND_PAGE_BYTE_CNT		0x1000
#define NAND_SPARE_BYTE_CNT		0x80

#define NAND_ECC_TYPE			ECC_TYPE_24BIT1KB
#define NAND_SECTOR_BYTE_CNT	0x200

#define NAND_RW_ADR_CYCLE_CNT	ADR_C5TFS0
#define NAND_E_ADR_CYCLE_CNT	ADR_C3TRS0

// Partition Info
#define PARTITION_CNT			1
#define PART_START_BLK			(NAND_BLK_CNT>>1)
#define PART_BLK_CNT			(NAND_BLK_CNT>>2)
#define PART_TYPE				UNFD_PART_FATU

extern void drvNAND_CHECK_FLASH_TYPE(NAND_DRIVER *pNandDrv);

U32 drvNAND_Init(void) 
{
	U32 u32_i, u32_RetVal;
	U8 *pu8;
	
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)nand_get_DrvContext_address();

	NANDMSG(nand_printf("\r\n--------------------\r\n"));
	NANDMSG(nand_printf("Hi! This is UNFD. \r\n"));
	NANDMSG(nand_printf("--------------------\r\n"));
	
	// pad switch
	nand_pads_switch(1);
	
	// set clock
	nand_clock_setting(NFIE_CLK_27M);

	// init device context to all 0
    pu8 = (U8*)pNandDrv;
	for(u32_i=0; u32_i<sizeof(NAND_DRIVER); u32_i++)
		pu8[u32_i] = 0;

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

	NC_ResetNandFlash();

	NC_ReadID();
	
#if (defined(NAND_DRV_LINUX) && NAND_DRV_LINUX)
	drvNAND_CHECK_FLASH_TYPE(pNandDrv);
#endif
	
	pNandDrv->u8_ECCType = NAND_ECC_TYPE;

	//----------------------------------
	// config NAND Driver - NAND Info
	//----------------------------------
	pNandDrv->u8_BlkPageCntBits = drvNand_CountBits(pNandDrv->u16_BlkPageCnt);
	pNandDrv->u8_PageByteCntBits = drvNand_CountBits(pNandDrv->u32_PageByteCnt);
	pNandDrv->u8_SpareByteCntBits = drvNand_CountBits(pNandDrv->u16_SpareByteCnt);
	pNandDrv->u8_SectorByteCntBits = drvNand_CountBits(pNandDrv->u32_SectorByteCnt);
	
	pNandDrv->u16_BlkPageCntMask = (1<<pNandDrv->u8_BlkPageCntBits)-1;
	pNandDrv->u32_PageByteCntMask = (1<<pNandDrv->u8_PageByteCntBits)-1;
	pNandDrv->u32_SectorByteCntMask = (1<<pNandDrv->u8_SectorByteCntBits)-1;
	pNandDrv->u16_SpareByteCntMask = (1<<pNandDrv->u8_SpareByteCntBits)-1;

	pNandDrv->u16_PageSectorCnt = pNandDrv->u32_PageByteCnt >> pNandDrv->u8_SectorByteCntBits;
	pNandDrv->u8_PageSectorCntBits = drvNand_CountBits(pNandDrv->u16_PageSectorCnt);
	pNandDrv->u16_PageSectorCntMask = (1<<pNandDrv->u8_PageSectorCntBits)-1;	
	pNandDrv->u16_SectorSpareByteCnt = pNandDrv->u16_SpareByteCnt >> pNandDrv->u8_PageSectorCntBits;
	pNandDrv->u8_SectorSpareByteCntBits = drvNand_CountBits(pNandDrv->u16_SectorSpareByteCnt);

	//---------------------------
	// config NC
	//---------------------------

	u32_RetVal = NC_Init();

	if(UNFD_ST_SUCCESS != u32_RetVal)
	{
		NANDMSG(nand_printf("ERROR: init FCIE fail, ErrCode: %Xh \r\n", (unsigned int)u32_RetVal)); 
		return u32_RetVal;
	}
	NANDMSG(nand_printf("NFIE RIU access ok! \r\n\n"));
	
	NC_Config();

	//---------------------------
	// config platform
	//---------------------------
	nand_platform_init();

	return UNFD_ST_SUCCESS;
}

U8 drvNand_CountBits(U32 u32_x)
{
    U8 u8_i = 0;

    while (u32_x) {
        u8_i++;
        u32_x = (u32_x >> 1);
    }
    return u8_i-1;
}

U8 drvNAND_ChkSum(U8 *pBuf, U16 u16ByteCnt)
{
    U16 u16Tmp;
    U32 u32Sum=0;
    for (u16Tmp=0; u16Tmp < u16ByteCnt; u16Tmp++) 
       u32Sum = ((u32Sum & 1) ? 0x80 : 0) + (u32Sum >> 1) + pBuf[u16Tmp];
     return (u32Sum);
}
