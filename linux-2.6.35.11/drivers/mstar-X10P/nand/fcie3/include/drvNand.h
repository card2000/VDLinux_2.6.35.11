/*===========================================================
 * MStar Semiconductor Inc.
 *
 * Nand Driver for FCIE3 - drvNand_v3.h
 *
 * History
 *    - initial version, 2009.06.07, Hill.Sung
 *      please modify the drvNand_platform.h for your platform.
 *
 *===========================================================*/

#ifndef NAND_DRV_V3
#define NAND_DRV_V3

typedef unsigned long		U32;
typedef unsigned short		U16;
typedef unsigned char		U8;
typedef long				S32;
typedef short				S16;
typedef char				S8;
 
#define BIT0				0x1
#define BIT1				0x2
#define BIT2				0x4
#define BIT3				0x8
#define BIT4				0x10
#define BIT5				0x20
#define BIT6				0x40
#define BIT7				0x80
#define BIT8				0x100
#define BIT9				0x200
#define BIT10				0x400
#define BIT11				0x800
#define BIT12				0x1000
#define BIT13				0x2000
#define BIT14				0x4000
#define BIT15				0x8000
#define BIT16				0x10000
#define BIT17				0x20000
#define BIT18				0x40000
#define BIT19				0x80000
#define BIT20				0x100000
#define BIT21				0x200000
#define BIT22				0x400000
#define BIT23				0x800000
#define BIT24				0x1000000
#define BIT25				0x2000000
#define BIT26				0x4000000
#define BIT27				0x8000000
#define BIT28				0x10000000
#define BIT29				0x20000000
#define BIT30				0x40000000
#define BIT31				0x80000000

#define SWAP16(x) \
	((U16)( \
	(((U16)(x) & (U16) 0x00ffU) << 8) | \
	(((U16)(x) & (U16) 0xff00U) >> 8) ))

#define SWAP32(x) \
	((U32)( \
	(((U32)(x) & (U32) 0x000000ffUL) << 24) | \
	(((U32)(x) & (U32) 0x0000ff00UL) <<  8) | \
	(((U32)(x) & (U32) 0x00ff0000UL) >>  8) | \
	(((U32)(x) & (U32) 0xff000000UL) >> 24) ))

#define SWAP64(x) \
	((U64)( \
	(U64)(((U64)(x) & (U64) 0x00000000000000ffULL) << 56) | \
	(U64)(((U64)(x) & (U64) 0x000000000000ff00ULL) << 40) | \
	(U64)(((U64)(x) & (U64) 0x0000000000ff0000ULL) << 24) | \
	(U64)(((U64)(x) & (U64) 0x00000000ff000000ULL) <<  8) | \
	(U64)(((U64)(x) & (U64) 0x000000ff00000000ULL) >>  8) | \
	(U64)(((U64)(x) & (U64) 0x0000ff0000000000ULL) >> 24) | \
	(U64)(((U64)(x) & (U64) 0x00ff000000000000ULL) >> 40) | \
	(U64)(((U64)(x) & (U64) 0xff00000000000000ULL) >> 56) ))

#ifdef BIG_ENDIAN // project team defined macro

#define cpu2le64(x)			SWAP64((x))
#define le2cpu64(x)			SWAP64((x))
#define cpu2le32(x)			SWAP32((x))
#define le2cpu32(x)			SWAP32((x))
#define cpu2le16(x)			SWAP16((x))
#define le2cpu16(x)			SWAP16((x))
#define cpu2be64(x)			((UINT64)(x))
#define be2cpu64(x)			((UINT64)(x))
#define cpu2be32(x)			((UINT32)(x))
#define be2cpu32(x)			((UINT32)(x))
#define cpu2be16(x)			((UINT16)(x))
#define be2cpu16(x)			((UINT16)(x))

#else	// Little_Endian

#define cpu2le64(x)			((UINT64)(x))
#define le2cpu64(x)			((UINT64)(x))
#define cpu2le32(x)			((UINT32)(x))
#define le2cpu32(x)			((UINT32)(x))
#define cpu2le16(x)			((UINT16)(x))
#define le2cpu16(x)			((UINT16)(x))
#define cpu2be64(x)			SWAP64((x))
#define be2cpu64(x)			SWAP64((x))
#define cpu2be32(x)			SWAP32((x))
#define be2cpu32(x)			SWAP32((x))
#define cpu2be16(x)			SWAP16((x))
#define be2cpu16(x)			SWAP16((x))

#endif	

//=====================================================================================
#include "drvNand_platform.h" // [CAUTION]: edit drvNand_platform.h for your platform
//=====================================================================================
#include "drvNand_ErrCodes.h"

//===========================================================
// NAND Info parameters
//===========================================================
#define NANDINFO_TAG		'M','S','T','A','R','S','E','M','I','U','N','F','D','C','I','S'
#define NANDINFO_TAG_LEN	16

#define VENDOR_RENESAS		0x07
#define VENDOR_ST			0x20
#define VENDOR_TOSHIBA		0x98
#define VENDOR_HYNIX		0xAD
#define VENDOR_SAMSUMG		0xEC

#define NAND_TYPE_MLC		BIT0 // 0: SLC, 1: MLC

//===========================================================
// driver parameters
//===========================================================
#define NAND_ID_BYTE_CNT	15

typedef struct NAND_CONFIG_FLAG
{
	//U32 IfPageSize512B : 1;
} nand_config_flag;

typedef struct _NAND_DRIVER
{
	U8 au8_ID[NAND_ID_BYTE_CNT];
	U8 u8_IDByteCnt;
	
	//-----------------------------
	// HAL parameters
	//-----------------------------
	#if NC_SEL_FCIE3
	volatile U16 u16_Reg1B_SdioCtrl;
	volatile U16 u16_Reg40_Signal;
	volatile U16 u16_Reg48_Spare;
	volatile U16 u16_Reg49_SpareSize;
	volatile U16 u16_Reg50_EccCtrl;
	#elif
	
	#endif
	
	U8 u8_OpCode_RW_AdrCycle;
	U8 u8_OpCode_Erase_AdrCycle;
	U8 u8_ECCType;
	
	NAND_DRIVER_PLAT NandDrvPlat_t;
	
	//-----------------------------
	// NAND Info
	//-----------------------------
	U16	u16_BlkCnt;
	U16 u16_BlkPageCnt;
	U32 u32_PageByteCnt;
	U16 u16_SpareByteCnt;
	U16 u16_PageSectorCnt;
	U32 u32_SectorByteCnt;
	U16 u16_SectorSpareByteCnt;
	U16 u16_ECCCodeByteCnt;
	
	U8 u8_BlkPageCntBits;
	U8 u8_PageByteCntBits;
	U8 u8_SpareByteCntBits;	
	U8 u8_PageSectorCntBits;
	U8 u8_SectorByteCntBits;
	U8 u8_SectorSpareByteCntBits;
		
	U16 u16_BlkPageCntMask;
	U32 u32_PageByteCntMask;
	U16 u16_SpareByteCntMask;
	U16 u16_PageSectorCntMask;
	U32 u32_SectorByteCntMask;
	
	//-----------------------------
	#if IF_IP_VERIFY
	U8 u8_IfECCTesting;
	#endif	
	
} NAND_DRIVER, *P_NAND_DRIVER;

//===========================================================
// ONIF Reg
//===========================================================
#define RIUBASE								0xBF000000
#define REG_BANK_ONIF						(0x88B00)
#define REG_OFFSET_SHIFT_BITS				2
#define ONIF_BASE							(RIUBASE+(REG_BANK_ONIF<<REG_OFFSET_SHIFT_BITS))
#define ONIF_CONFIG0						(ONIF_BASE+(0x00<<REG_OFFSET_SHIFT_BITS))
#define R_PIN_RPZ							BIT(6)

//===========================================================
// exposed APIs
//===========================================================
U32 drvNAND_Init(void);

//===========================================================
// Linux functions
//===========================================================
#if defined(NAND_DRV_LINUX) && NAND_DRV_LINUX

U32 drvNAND_ChipSel(U32 u32NandNo);
void drvNAND_SetCLE(U8 u8Level);
void drvNAND_SetALE(U8 u8Level);
U32 drvNAND_SetCEZ(U8 u8Level);
void drvNAND_SetWP(U8 u8Level);
U32 drvNAND_WaitReady(U32 u32USec);

void drvNAND_CmdReadOOB(U32 u32Row, U32 u32Col);

U8 drvNAND_ReadByte(void);
void drvNAND_WriteByte(U8 u8Byte);
U16 drvNAND_ReadWord(void);
void drvNAND_WriteWord(U16 u16Word);
void drvNAND_ReadBuf(U8 *u8Buf, U16 u16Len);
void drvNAND_WriteBuf(U8 *u8Buf, U16 u16Len);

int drvNand_CheckECC(void);
U32 drvNand_Read_Status(void);
#endif

//===========================================================
// internal used functions
//===========================================================
U32 drvNand_get_DrvContext_address(void);
U8 drvNand_CountBits(U32 u32_x);
U8 drvNAND_ChkSum(U8 *pBuf, U16 u16ByteCnt);

//===========================================================
// HAL functions
//===========================================================
U32 NC_Init(void);
U32 NC_ReadPages(U32 u32_PhyRowIdx, U8 *pu8_DataBuf, U8 *pu8_SpareBuf, U32 u32_PageCnt);
U32 NC_WritePages(U32 u32_PhyRowIdx, U8 *pu8_DataBuf, U8 *pu8_SpareBuf, U32 u32_PageCnt);
U32 NC_ReadSectors(U32 u32_PhyRowIdx, U8 u8_SectorInPage, U8 *pu8_DataBuf, U8 *pu8_SpareBuf, U32 u32_SectorCnt);
U32 NC_WriteSector(U32 u32_PhyRowIdx, U8 u8_SectorInPage, U8 *pu8_DataBuf, U8 *pu8_SpareBuf);
U32 NC_ReadID(void);
U32 NC_EraseBlk(U32 u32PhyRowIdx);
U32 NC_ResetNandFlash(void);
U32 NC_Read_RandomIn(U32 u32PhyRow, U32 u32Col, U8 *pu8DataBuf, U32 u32DataByteCnt);
U32 NC_Write_RandomOut(U32 u32_PhyRow, U32 u32_Col, U8 *pu8_DataBuf, U32 u32_DataByteCnt);
U32 NC_GetECCBits(void);
U32  NC_ResetFCIE(void);

void NC_Config(void);
void NC_SetCIFD(U8 *pu8_Buf, U32 u32_CIFDPos, U32 u32_ByteCnt);
void NC_GetCIFD(U8 *pu8_Buf, U32 u32_CIFDPos, U32 u32_ByteCnt);
void NC_SetCIFD_Const(U8 u8_Data, U32 u32_CIFDPos, U32 u32_ByteCnt);

U32 NC_WritePage_RIUMode(U32 u32_PhyRowIdx, U8 *pu8_DataBuf, U8 *pu8_SpareBuf);
U32 NC_ReadSector_RIUMode(U32 u32_PhyRowIdx, U8 u8_SectorInPage, U8 *pu8_DataBuf, U8 *pu8_SpareBuf);

#endif // NAND_DRV_V3
 
