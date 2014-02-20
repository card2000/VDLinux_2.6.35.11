#ifndef NAND_DRV_REGISTERS
#define NAND_DRV_REGISTERS

//------------------------------------------------------------------
#define ECC_TYPE_RS							1
#define ECC_TYPE_4BIT						2
#define ECC_TYPE_8BIT						3
#define ECC_TYPE_12BIT						4
#define ECC_TYPE_16BIT						5
#define ECC_TYPE_20BIT						6
#define ECC_TYPE_24BIT						7
#define ECC_TYPE_24BIT1KB					8
#define ECC_TYPE_32BIT1KB					9

#define ECC_CODE_BYTECNT_RS					10
#define ECC_CODE_BYTECNT_4BIT				7
#define ECC_CODE_BYTECNT_8BIT				13
#define ECC_CODE_BYTECNT_12BIT				20
#define ECC_CODE_BYTECNT_16BIT				26
#define ECC_CODE_BYTECNT_20BIT				33
#define ECC_CODE_BYTECNT_24BIT				39
#define ECC_CODE_BYTECNT_24BIT1KB			42
#define ECC_CODE_BYTECNT_32BIT1KB			56

//------------------------------------------------------------------
#define IF_SPARE_AREA_DMA					0 // [CAUTION]

//------------------------------------------------------------------
#define GET_REG_ADDR(x, y)					(x+((y)<<REG_OFFSET_SHIFT_BITS))

#define NC_MIE_EVENT						GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x00)
#define NC_MIE_INT_EN						GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x01)
#define NC_MMA_PRI_REG						GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x02)
#define NC_MIU_DMA_SEL						GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x03)
#define NC_CARD_DET							GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x07)
#define NC_FORCE_INT						GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x08)
#define NC_PATH_CTL							GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x0A)
#define NC_JOB_BL_CNT						GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x0B)
#define NC_TR_BK_CNT						GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x0C)
#define NC_SDIO_CTL							GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x1B)
#define NC_SDIO_ADDR0						GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x1C)
#define NC_SDIO_ADDR1						GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x1D)
#define NC_R2N_CTRL_GET						GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x1F)
#define NC_R2N_CTRL_SET						GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x20)
#define NC_R2N_DATA_RD						GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x21)
#define NC_R2N_DATA_WR						GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x22)
#define NC_CIF_FIFO_CTL						GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x25)
#define NC_MIU_OFFSET						GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x2E)
#define NC_TEST_MODE						GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x30)
#define NC_CELL_DELAY						GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x37)
#define NC_MIU_WPRT_L1						GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x38)
#define NC_MIU_WPRT_L0						GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x39)
#define NC_MIU_WPRT_H1						GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x3A)
#define NC_MIU_WPRT_H0						GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x3B)
#define NC_MIU_WPRT_ER1						GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x3C)
#define NC_MIU_WPRT_ER0						GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x3D)
#define NC_FCIE_VERSION						GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x3F)
#define NC_SIGNAL							GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x40)
#define NC_WIDTH							GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x41)
#define NC_STAT_CHK							GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x42)
#define NC_AUXREG_ADR						GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x43)
#define NC_AUXREG_DAT						GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x44)
#define NC_CTRL								GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x45)
#define NC_ST_READ							GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x46)
#define NC_PART_MODE						GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x47)
#define NC_SPARE							GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x48)
#define NC_SPARE_SIZE						GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x49)
#define NC_ADDEND							GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x4A)
#define NC_SIGN_DAT							GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x4B)
#define NC_SIGN_ADR							GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x4C)
#define NC_SIGN_CTRL						GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x4D)
#define NC_SPARE_DMA_ADR0					GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x4E)
#define NC_SPARE_DMA_ADR1					GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x4F)
#define NC_ECC_CTRL							GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x50)
#define NC_ECC_STAT0						GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x51)   
#define NC_ECC_STAT1						GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x52)   
#define NC_ECC_STAT2						GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x53)   
#define NC_ECC_LOC							GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x54)   
#define NC_RAND_R_CMD						GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x55)   
#define NC_RAND_W_CMD						GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x56)   

#define NC_CIFD_ADDR(u16_pos)				GET_REG_ADDR(FCIE_NC_CIFD_BASE, u16_pos) // 256 x 16 bits
#define NC_CIFD_BYTE_CNT					0x200 // 256 x 16 bits

#define NC_MAX_SECTOR_BYTE_CNT				(BIT12-1)
#define NC_MAX_SECTOR_SPARE_BYTE_CNT		(BIT8-1)
#define NC_MAX_TOTAL_SPARE_BYTE_CNT			(BIT11-1)

//------------------------------------------------------------------
/* NC_MIE_EVENT 0x00 */
/* NC_MIE_INT_EN 0x01 */
#define BIT_MMA_DATA_END					BIT0  
#define BIT_NC_JOB_END						BIT9
#define BIT_NC_R2N_RDY						BIT10
#define BIT_NC_R2N_ECC						BIT12
/* NC_MMA_PRI_REG 0x02 */
#define BIT_NC_DMA_DIR_W					BIT2
/* NC_PATH_CTL 0x0A */
#define BIT_MMA_EN							BIT0  
#define BIT_NC_EN							BIT5
/* NC_SDIO_CTL 0x1B */
#define MASK_SDIO_BLK_SIZE_MASK				(BIT12-1)
#define BIT_SDIO_BLK_MODE					BIT15
/* NC_R2N_CTRL_GET 0x1F */
#define BIT_RIU_RDY_MMA						BIT0 
/* NC_R2N_CTRL_SET 0x20 */
#define BIT_R2N_MODE_EN						BIT0
#define BIT_R2N_DI_START					BIT1
#define BIT_R2N_DI_EN						BIT2
#define BIT_R2N_DI_END						BIT3
#define BIT_R2N_DO_START					BIT4
#define BIT_R2N_DO_EN						BIT5
#define BIT_R2N_DO_END						BIT6
/* NC_CIF_FIFO_CTL 0x25 */
#define BIT_CIFD_RD_REQ						BIT1 
/* NC_TEST_MODE 0x30 */
#define BIT_FCIE_BIST_FAIL					(BIT0|BIT1|BIT2|BIT3) 
#define BIT_FCIE_DEBUG_MODE					(BIT10|BIT9|BIT8) 
#define BIT_FCIE_SOFT_RST					BIT12 
#define BIT_FCIE_PPFIFO_CLK					BIT14
/* NC_FCIE_VERSION 0x3F */
#define BIT_NFIE_INSIDE						BIT5  
/* NC_SIGNAL 0x40 */
#define BIT_NC_CE_SEL_MASK					(BIT2-1)
#define BIT_NC_CE_H							BIT2  
#define BIT_NC_CE_AUTO						BIT3
#define BIT_NC_WP_H							BIT4
#define BIT_NC_WP_AUTO						BIT5
#define BIT_NC_CHK_RB_HIGH					BIT6
#define BIT_NC_CHK_RB_EDGEn					BIT7
#define DEF_REG0x40_VAL						BIT_NC_CE_H
/* NC_WIDTH 0x41 */
#define BIT_NC_DEB_SEL						(BIT12|BIT13|BIT14)
#define BIT_NC_BCH_DEB_SEL					BIT15
/* NC_CTRL 0x45 */
#define BIT_NC_JOB_START					BIT0
#define BIT_NC_CIFD_ACCESS					BIT1
#define BIT_NC_IF_DIR_W						BIT3
/* NC_ST_READ 0x46 */
#define BIT_ST_READ_FAIL					BIT0
#define BIT_ST_READ_BUSYn					BIT6
#define BIT_ST_READ_PROTECTn				BIT7
/* NC_PART_MODE 0x47*/
#define BIT_PARTIAL_MODE_EN					BIT0
#define BIT_START_SECTOR_CNT_MASK			(BIT6|BIT5|BIT4|BIT3|BIT2|BIT1)
#define BIT_START_SECTOR_CNT_SHIFT			1
#define BIT_START_SECTOR_IDX_MASK			(BIT12|BIT11|BIT10|BIT9|BIT8|BIT7)
#define BIT_START_SECTOR_IDX_SHIFT			7
/* NC_SPARE 0x48 */
#define BIT_NC_SECTOR_SPARE_SIZE_MASK		(BIT8-1)
#define BIT_NC_SPARE_DEST_MIU				BIT8
#define BIT_NC_SPARE_NOT_R_IN_CIFD			BIT9
#define BIT_NC_RANDOM_RW_OP_EN				BIT11
#define BIT_NC_HW_AUTO_RANDOM_CMD_DISABLE	BIT12
#define BIT_NC_ONE_COL_ADDR					BIT13
/* NC_SIGN_CTRL 0x4D*/
#define BIT_NC_SIGN_CHECK_EN				BIT0
#define BIT_NC_SIGN_STOP_RUN				BIT1
#define BIT_NC_SIGN_CLR_STATUS				BIT2
#define BIT_NC_SIGN_DAT0_MISS				BIT3
#define BIT_NC_SIGN_DAT1_MISS				BIT4
/* NC_ECC_CTRL 0x50*/
#define BIT_NC_PAGE_SIZE_MASK				(BIT3-1)
#define BIT_NC_PAGE_SIZE_512Bn				BIT_NC_PAGE_SIZE_MASK
#define BIT_NC_PAGE_SIZE_2KB				BIT0
#define BIT_NC_PAGE_SIZE_4KB				BIT1
#define BIT_NC_PAGE_SIZE_8KB				(BIT1|BIT0)
#define BIT_NC_PAGE_SIZE_16KB				BIT2
#define BIT_NC_PAGE_SIZE_32KB				(BIT2|BIT0)
#define BIT_NC_ECC_TYPE_MASK				(BIT3|BIT4|BIT5|BIT6)
#define BIT_NC_ECC_TYPE_4b512Bn				BIT_NC_ECC_TYPE_MASK
#define BIT_NC_ECC_TYPE_8b512B				BIT3
#define BIT_NC_ECC_TYPE_12b512B				BIT4
#define BIT_NC_ECC_TYPE_24b1KB				(BIT5|BIT4)
#define BIT_NC_ECC_TYPE_32b1KB				(BIT5|BIT4|BIT3)
#define BIT_NC_ECC_TYPE_RS					BIT6
#define BIT_NC_ECCERR_NSTOP					BIT7
#define BIT_NC_DYNAMIC_OFFCLK				BIT8
#define BIT_NC_ALL0xFF_ECC_CHECK			BIT9
#define BIT_NC_BYPASS_ECC					BIT10
/* NC_ECC_STAT0 0x51*/
#define BIT_NC_ECC_FAIL						BIT0
#define BIT_NC_ECC_MAX_BITS_MASK			(BIT6|BIT5|BIT4|BIT3|BIT2|BIT1)
/* NC_ECC_STAT2 0x53*/
#define BIT_NC_ECC_FLGA_MASK				(BIT1|BIT0)
#define BIT_NC_ECC_FLGA_NOERR				0
#define BIT_NC_ECC_FLGA_CORRECT				BIT0
#define BIT_NC_ECC_FLGA_FAIL				BIT1
#define BIT_NC_ECC_FLGA_CODE				BIT_NC_ECC_FLGA_MASK
#define BIT_NC_ECC_CORRECT_CNT				(BIT7|BIT6|BIT5|BIT4|BIT3|BIT2)
#define BIT_NC_ECC_CNT_SHIFT				2
#define BIT_NC_ECC_SEL_LOC_MASK				(BIT12|BIT11|BIT10|BIT9|BIT8)
#define BIT_NC_ECC_SEL_LOC_SHIFT			8

//------------------------------------------------------------------
// AUX Reg Address
#define AUXADR_CMDREG8						0x08
#define AUXADR_CMDREG9						0x09
#define AUXADR_CMDREGA						0x0A
#define AUXADR_ADRSET						0x0B 
#define AUXADR_RPTCNT						0x18 // Pepeat Count
#define AUXADR_RAN_CNT						0x19 
#define AUXADR_RAN_POS						0x1A // offset
#define AUXADR_ST_CHECK						0x1B
#define AUXADR_IDLE_CNT						0x1C
#define AUXADR_INSTQUE						0x20

// OP Codes: Command
#define CMD_0x00							0x00 
#define CMD_0x30							0x01
#define CMD_0x35							0x02
#define CMD_0x90							0x03
#define CMD_0xFF							0x04
#define CMD_0x80							0x05
#define CMD_0x10							0x06
#define CMD_0x15							0x07
#define CMD_0x85							0x08
#define CMD_0x60							0x09
#define CMD_0xD0							0x0A
#define CMD_0x05							0x0B
#define CMD_0xE0							0x0C
#define CMD_0x70							0x0D
#define CMD_0x50							0x0E
#define CMD_0x01							0x0F
#define CMD_REG8							0x10
#define CMD_REG9							0x11
#define CMD_REGA							0x12

// OP Code: Address
#define OP_ADR_CYCLE_00						(4<<4)
#define OP_ADR_CYCLE_01						(5<<4)
#define OP_ADR_CYCLE_10						(6<<4)
#define OP_ADR_CYCLE_11						(7<<4)
#define OP_ADR_TYPE_COL						(0<<2)
#define OP_ADR_TYPE_ROW						(1<<2)
#define OP_ADR_TYPE_FUL						(2<<2)
#define OP_ADR_TYPE_ONE						(3<<2)
#define OP_ADR_SET_0						(0<<0)
#define OP_ADR_SET_1						(1<<0)
#define OP_ADR_SET_2						(2<<0)
#define OP_ADR_SET_3						(3<<0)

#define ADR_C2TRS0							(OP_ADR_CYCLE_00|OP_ADR_TYPE_ROW|OP_ADR_SET_0)
#define ADR_C2T1S0							(OP_ADR_CYCLE_00|OP_ADR_TYPE_ONE|OP_ADR_SET_0)
#define ADR_C3TRS0							(OP_ADR_CYCLE_01|OP_ADR_TYPE_ROW|OP_ADR_SET_0)
#define ADR_C3TFS0							(OP_ADR_CYCLE_00|OP_ADR_TYPE_FUL|OP_ADR_SET_0)
#define ADR_C4TFS0							(OP_ADR_CYCLE_01|OP_ADR_TYPE_FUL|OP_ADR_SET_0)
#define ADR_C4TFS1							(OP_ADR_CYCLE_01|OP_ADR_TYPE_FUL|OP_ADR_SET_1)
#define ADR_C5TFS0							(OP_ADR_CYCLE_10|OP_ADR_TYPE_FUL|OP_ADR_SET_0)
#define ADR_C6TFS0							(OP_ADR_CYCLE_11|OP_ADR_TYPE_FUL|OP_ADR_SET_0)

// OP Code: Action
#define ACT_WAITRB							0x80 	
#define ACT_CHKSTATUS						0x81    
#define ACT_WAIT_IDLE						0x82	    
#define ACT_WAIT_MMA						0x83     
#define ACT_BREAK							0x88
#define ACT_SER_DOUT						0x90 // for column addr == 0       	
#define ACT_RAN_DOUT						0x91 // for column addr != 0	
#define ACT_SER_DIN							0x98 // for column addr == 0       
#define ACT_RAN_DIN							0x99 // for column addr != 0
#define ACT_PAGECOPY_US						0xA0 	 
#define ACT_PAGECOPY_DS						0xA1 	
#define ACT_REPEAT							0xB0     

#endif // NAND_DRV_REGISTERS
