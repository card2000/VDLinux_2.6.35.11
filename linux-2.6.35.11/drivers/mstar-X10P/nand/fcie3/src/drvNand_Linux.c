//=======================================================================
//  MStar Semiconductor - Unified Nand Flash Driver
//
//  drvNand_Linux.c - Storage Team, 2009/10/13 
//
//  Design Notes:
//    1. 2009/10/13 - for Linux
//
//=======================================================================

#include "../include/drvNand.h"

#if NAND_DRIVER_OS_LINUX

#include <linux/delay.h>

#define KSEG02KSEG1(addr)	((void *)((U32)(addr)|0x20000000))
#define KSEG12KSEG0(addr)	((void *)((U32)(addr)&~0x20000000))

U32 NC_WaitComplete(U16 u16_WaitEvent, U32 u32_MicroSec);

static U32 u32CurNandNo = 0;
U8 u8MainBuf[8192] __attribute((aligned(128)));
U8 u8SpareBuf[432] __attribute((aligned(128)));
U32 u32CIFDIdx = 0;
U16 u16ByteIdxofPage;
U32 u32WriteLen;

extern U32 u32CurRow;
extern U32 u32CurCol;

extern void nand_lock_fcie(void);
extern void nand_unlock_fcie(void);
#define NC_LOCK_FCIE()		nand_lock_fcie()
#define NC_UNLOCK_FCIE()	nand_unlock_fcie()

U32 drvNAND_ChipSel(U32 u32NandNo)
{
	if( u32NandNo > 3 )
	{
		NANDMSG(nand_printf("\n%s(): Wrong Chip No %d.\n", __FUNCTION__, (int)u32NandNo));
		return 1;
	}
	
	u32CurNandNo = u32NandNo;

	return UNFD_ST_SUCCESS;
}

void drvNAND_SetCLE(U8 u8Level)
{
	if( u8Level )
	{
	}	
}

void drvNAND_SetALE(U8 u8Level)
{
	if( u8Level )
	{
	}
}

U32 drvNAND_SetCEZ(U8 u8Level)
{
	REG_CLR_BITS_UINT16(NC_SIGNAL, BIT_NC_WP_AUTO | BIT_NC_CE_AUTO);

	if( u8Level )
	{
		#if defined(NAND_CE_GPIO) && NAND_CE_GPIO
		drvNAND_SetCEGPIO(1);
		#else
		REG_SET_BITS_UINT16(NC_SIGNAL, BIT_NC_CE_H);
		#endif
	}
	else
	{
		#if defined(NAND_CE_GPIO) && NAND_CE_GPIO
		drvNAND_SetCEGPIO(0);
		#else
		REG_CLR_BITS_UINT16(NC_SIGNAL, BIT_NC_CE_H);
		#endif
	}

	return UNFD_ST_SUCCESS;
}

void drvNAND_SetWP(U8 u8Level)
{
	REG_CLR_BITS_UINT16(NC_SIGNAL, BIT_NC_WP_AUTO | BIT_NC_CE_AUTO);

	if( u8Level )
	{
		REG_SET_BITS_UINT16(NC_SIGNAL, BIT_NC_WP_H);
		#if defined(NAND_WP_GPIO) && NAND_WP_GPIO
		REG_SET_BITS_UINT16(ONIF_CONFIG0, R_PIN_RPZ);
		//drvNAND_SetWPGPIO(1);
		#endif
	}
	else
	{
		#if defined(NAND_WP_GPIO) && NAND_WP_GPIO
		REG_CLR_BITS_UINT16(ONIF_CONFIG0, R_PIN_RPZ);
		//drvNAND_SetWPGPIO(0);
		#else
		REG_CLR_BITS_UINT16(NC_SIGNAL, BIT_NC_WP_H);
		#endif
	}
}

U32 drvNAND_WaitReady(U32 u32USec)
{
	NC_LOCK_FCIE();
	
	nand_pads_switch(1);

	REG_WRITE_UINT16(NC_AUXREG_ADR ,AUXADR_INSTQUE);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_BREAK<<8) | ACT_WAITRB);

	#if defined(NAND_CE_GPIO) && NAND_CE_GPIO
	drvNAND_SetCEGPIO(0);
	#endif

	REG_WRITE_UINT16(NC_CTRL, BIT_NC_JOB_START);

	if( NC_WaitComplete(BIT_NC_JOB_END, u32USec * 1000) == (u32USec*1000) )
	{
		#if defined(NAND_CE_GPIO) && NAND_CE_GPIO
		drvNAND_SetCEGPIO(1);
		#endif
		NC_UNLOCK_FCIE();
		return 1;
	}

	#if defined(NAND_CE_GPIO) && NAND_CE_GPIO
	drvNAND_SetCEGPIO(1);
	#endif
	NC_UNLOCK_FCIE();
	return UNFD_ST_SUCCESS;
}

void drvNAND_CmdReadOOB(U32 u32Row, U32 u32Col)
{
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNand_get_DrvContext_address();
	
	u32CIFDIdx = 0;
	u16ByteIdxofPage = 0;

	NC_Read_RandomIn(u32Row, u32Col, u8SpareBuf, pNandDrv->u16_SpareByteCnt);
}


U8 drvNAND_ReadByte(void)
{
	U8 u8ByteData = 0;

	NC_GetCIFD(&u8ByteData, u32CIFDIdx, 1);
	
	u32CIFDIdx++;
	
	return u8ByteData;
}

void drvNAND_WriteByte(U8 u8Byte)
{
	if( u8Byte )
	{
	}
}

U16 drvNAND_ReadWord(void)
{
	return UNFD_ST_SUCCESS;
}

void drvNAND_WriteWord(U16 u16Word)
{
	if( u16Word )
	{
	}
}

void drvNAND_ReadBuf(U8 *u8Buf, U16 u16Len)
{
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNand_get_DrvContext_address();
	int spareoffset = pNandDrv->u32_PageByteCnt;
	U8 *pu8Buf;

	u16ByteIdxofPage = u16Len;

	//printk(" $$$$$$$$$$$ drvNAND_ReadBuf  u8Buf %08x, u16Len %d\n",u8Buf ,  u16Len);

	if( ((U32)u8Buf) >= 0xC0000000 )
		pu8Buf = u8MainBuf;
	else
		pu8Buf = u8Buf;

	if( u16Len > pNandDrv->u16_SpareByteCnt )
	{
		if( (u16Len % 0x200 ) != 0 )
		{
			NC_ReadPages(u32CurRow, pu8Buf, pu8Buf+spareoffset, 1);
		}
		else
		{
			NC_ReadPages(u32CurRow, pu8Buf, u8SpareBuf, 1);
		}
	}
	else
	{
		NC_Read_RandomIn(u32CurRow, spareoffset, pu8Buf, u16Len);
	}
	
	if( u8Buf != pu8Buf )
		memcpy(u8Buf, pu8Buf, u16Len);

}

void drvNAND_WriteBuf(U8 *u8Buf, U16 u16Len)
{
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNand_get_DrvContext_address();
	int spareoffset = pNandDrv->u32_PageByteCnt;
	U8 *pu8Buf;

	u32WriteLen += u16Len;
	u16ByteIdxofPage += u16Len;

	//printk(" @@@@@@@ drvNAND_WriteBuf u8Buf %08x  u16Len %d\n",u8Buf,  u16Len);

	if( ((U32)u8Buf) >= 0xC0000000 )
	{
		memcpy(u8MainBuf, u8Buf, u16Len);
		pu8Buf = u8MainBuf;
	}
	else
		pu8Buf = u8Buf;

	if( u16Len >= pNandDrv->u32_PageByteCnt )
	{
		if( u16Len > pNandDrv->u32_PageByteCnt )
		{
			NC_WritePages(u32CurRow, pu8Buf, pu8Buf+spareoffset, 1);
		}
		else
		{
			memcpy(u8MainBuf, u8Buf, u16Len);
		}
	}
	else
	{
		if((u32WriteLen==0) && (u16ByteIdxofPage>=pNandDrv->u32_PageByteCnt))
		{
			memset(u8MainBuf, 0xFF, pNandDrv->u32_PageByteCnt);
			memset(u8SpareBuf, 0xFF, pNandDrv->u16_SpareByteCnt);
		}
		memcpy(u8SpareBuf, u8Buf, u16Len);

		if( u32WriteLen == (pNandDrv->u32_PageByteCnt + pNandDrv->u16_SpareByteCnt) )
			NC_WritePages(u32CurRow, u8MainBuf, u8SpareBuf, 1);
	}
	
}

int drvNand_CheckECC(void)
{
	U16 u16RegValue = 0;
	
	REG_READ_UINT16(NC_ECC_STAT2, u16RegValue);
	
	if( u16RegValue & BIT_NC_ECC_FLGA_FAIL ) //Uncorrectable ECC error
		return -1;

	if( u16RegValue & BIT_NC_ECC_FLGA_CORRECT ) //Uncorrectable ECC error
		printk("ecc correction !!!!!!!!!!!!\n");
		
	//No ECC error or Correctable error

	return 0;
}

U32 drvNand_Read_Status(void)
{
	U8 u8Status = 0;
	U16 u16RegValue = 0;

	NC_LOCK_FCIE();
	
	nand_pads_switch(1);

	REG_WRITE_UINT16(NC_AUXREG_ADR, AUXADR_INSTQUE);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_CHKSTATUS<<8) | CMD_0x70);
	REG_WRITE_UINT16(NC_AUXREG_DAT, (ACT_BREAK<<8) | ACT_BREAK);

	#if defined(NAND_CE_GPIO) && NAND_CE_GPIO
	drvNAND_SetCEGPIO(0);
	#endif
	
	REG_WRITE_UINT16(NC_CTRL, BIT_NC_JOB_START);

	if(NC_WaitComplete(BIT_NC_JOB_END, DELAY_100ms_in_us) == DELAY_100ms_in_us)
	{
		NANDMSG(nand_printf("%s: Job End Timout\n", __FUNCTION__));

		#if defined(NAND_CE_GPIO) && NAND_CE_GPIO
		drvNAND_SetCEGPIO(1);
		#endif
		NC_UNLOCK_FCIE();

		return UNFD_ST_ERR_R_TIMEOUT;
	}

	REG_READ_UINT16(NC_ST_READ, u16RegValue);
	u8Status = u16RegValue;
	NC_SetCIFD(&u8Status, 0, 1);

	#if defined(NAND_CE_GPIO) && NAND_CE_GPIO
	drvNAND_SetCEGPIO(1);
	#endif

	NC_UNLOCK_FCIE();

	return UNFD_ST_SUCCESS;
}

#endif

