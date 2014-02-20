//===========================================================
// error codes
//===========================================================
#define  UNFD_ST_SUCCESS					0

#define  UNFD_ERRCODE_GROUP_MASK			0xF0000000

//===========================================================
// for LINUX functions
//===========================================================
#if defined(NAND_DRV_LINUX) && NAND_DRV_LINUX
#define  UNFD_ST_LINUX						0xC0000000
#define  UNFD_ST_ERR_NO_NFIE				(0x01 | UNFD_ST_LINUX)
#define  UNFD_ST_ERR_E_TIMEOUT				(0x07 | UNFD_ST_LINUX)
#define  UNFD_ST_ERR_W_TIMEOUT				(0x08 | UNFD_ST_LINUX)
#define  UNFD_ST_ERR_R_TIMEOUT				(0x09 | UNFD_ST_LINUX)
#define  UNFD_ST_ERR_ECC_FAIL				(0x0F | UNFD_ST_LINUX)
#define  UNFD_ST_ERR_RST_TIMEOUT			(0x12 | UNFD_ST_LINUX)
#define  UNFD_ST_ERR_MIU_RPATCH_TIMEOUT		(0x13 | UNFD_ST_LINUX)
#define  UNFD_ST_ERR_HAL_R_INVALID_PARAM	(0x14 | UNFD_ST_LINUX)
#define  UNFD_ST_ERR_HAL_W_INVALID_PARAM	(0x15 | UNFD_ST_LINUX)
#define  UNFD_ST_ERR_R_TIMEOUT_RM			(0x16 | UNFD_ST_LINUX)
#define  UNFD_ST_ERR_ECC_FAIL_RM			(0x17 | UNFD_ST_LINUX)
#define  UNFD_ST_ERR_W_TIMEOUT_RM			(0x18 | UNFD_ST_LINUX)
#endif

//===========================================================
// for HAL functions
//===========================================================
#define  UNFD_ST_RESERVED					0xE0000000
#define  UNFD_ST_ERR_E_FAIL					(0x01 | UNFD_ST_RESERVED)
#define  UNFD_ST_ERR_W_FAIL					(0x02 | UNFD_ST_RESERVED)
#define  UNFD_ST_ERR_E_BUSY					(0x03 | UNFD_ST_RESERVED)
#define  UNFD_ST_ERR_W_BUSY					(0x04 | UNFD_ST_RESERVED)
#define  UNFD_ST_ERR_E_PROTECTED			(0x05 | UNFD_ST_RESERVED)
#define  UNFD_ST_ERR_W_PROTECTED			(0x06 | UNFD_ST_RESERVED)

//===========================================================
// for IP_Verify functions
//===========================================================
#define  UNFD_ST_IP							0xF0000000
#define  UNFD_ST_ERR_UNKNOWN_ID				(0x01 | UNFD_ST_IP)
#define  UNFD_ST_ERR_DATA_CMP_FAIL			(0x02 | UNFD_ST_IP)
#define  UNFD_ST_ERR_INVALID_PARAM			(0x03 | UNFD_ST_IP)
#define  UNFD_ST_ERR_INVALID_ECC_REG51		(0x04 | UNFD_ST_IP)
#define  UNFD_ST_ERR_INVALID_ECC_REG52		(0x05 | UNFD_ST_IP)
#define  UNFD_ST_ERR_INVALID_ECC_REG53		(0x06 | UNFD_ST_IP)
#define  UNFD_ST_ERR_SIGNATURE_FAIL			(0x07 | UNFD_ST_IP)
