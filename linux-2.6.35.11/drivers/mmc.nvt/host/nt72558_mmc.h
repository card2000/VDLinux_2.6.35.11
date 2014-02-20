
#ifndef __NT72558_MMC_H__
#define __NT72558_MMC_H__

#define VOID                void
#define FALSE               0
#define TRUE                1

typedef char                CHAR;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef unsigned long       DWORD;

typedef unsigned long long  ULONGLONG;
typedef long long           LONGLONG;
typedef ULONGLONG * PULONGLONG;
typedef LONGLONG *  PLONGLONG;

typedef short               SHORT;
typedef long                LONG;
typedef int                 INT;

typedef unsigned long       ULONG;
typedef unsigned short      USHORT;
typedef unsigned int        UINT;


typedef CHAR *      PCHAR;
typedef SHORT *     PSHORT;
typedef LONG *      PLONG;
typedef VOID *      PVOID;


// cat't define as (WORD *)!!!
// typedef WORD *      PWORD;
// typedef BYTE *      PBYTE;
// typedef DWORD *     PDWORD;

#ifndef NULL
#define NULL        0
#endif //

#define SDCLK_SOURCE    (52000000 * 4)

#define CLOCKRATE_MIN			400000
#define CLOCKRATE_MAX			52000000

#define REG_NFC_BASE                            (0xBC048000)

#define REG_NFC_SYS_CTRL                        (*(volatile unsigned long *)(REG_NFC_BASE+0x5c))
#define Emmc_Sel                                (1<<7)

#define REG_FCR_BASE                            (0xBC048200)

#define REG_FCR_FUNC_CTRL                       (*(volatile DWORD*)(REG_FCR_BASE+0x00))
#define REG_FCR_CPBLT                           (*(volatile DWORD*)(REG_FCR_BASE+0x04))
#define REG_FCR_SD_2nd_FUNC_CTRL                (*(volatile DWORD*)(REG_FCR_BASE+0x08))
#define REG_FCR_FUNC_CTRL_1                     (*(volatile DWORD*)(REG_FCR_BASE+0x0c))


#define REG_FCR_AHB_PREFETCH_ENABLE             (*(volatile DWORD*)(0xBA050048))
#define FCR_AHB_PREFETCH_ENABLE_BIT             (1<<0)

#define FCR_FUNC_CTRL_SW_MS_CD                  (1 << 27)
#define FCR_FUNC_CTRL_SW_SD_CD                  (1 << 26)
#define FCR_FUNC_CTRL_SW_SD_WP                  (1 << 25)
#define FCR_FUNC_CTRL_SW_CDWP_ENABLE            (1 << 24)
#define FCR_FUNC_CTRL_LITTLE_ENDIAN             (1 << 23)
#define FCR_FUNC_CTRL_SD_FLEXIBLE_CLK           (1 << 22)
#define FCR_FUNC_CTRL_AHB_MAX_BURST_SIZE(n)     (((n) & 3) << 20)
#define FCR_FUNC_CTRL_AHB_MAX_BURST_SIZE_MASK   FCR_FUNC_CTRL_AHB_MAX_BURST_SIZE(-1)
#define FCR_FUNC_CTRL_AHB_MAX_BURST_SIZE_512    FCR_FUNC_CTRL_AHB_MAX_BURST_SIZE(3)
#define FCR_FUNC_CTRL_AHB_MAX_BURST_SIZE_256    FCR_FUNC_CTRL_AHB_MAX_BURST_SIZE(2)
#define FCR_FUNC_CTRL_AHB_MAX_BURST_SIZE_128    FCR_FUNC_CTRL_AHB_MAX_BURST_SIZE(1)
#define FCR_FUNC_CTRL_AHB_MAX_BURST_SIZE_64     FCR_FUNC_CTRL_AHB_MAX_BURST_SIZE(0)
#define FCR_FUNC_CTRL_INT_OPT_STAT              (1 << 19)
#define FCR_FUNC_CTRL_INT_OPT_DMA               (1 << 18)
#define FCR_FUNC_CTRL_SD_PULLUP_RESISTOR        (1 << 17)
#define FCR_FUNC_CTRL_MMC_8BIT                  (1 << 16)
#define FCR_FUNC_CTRL_CD_DEBOUNCE_TIME(n)       (((n) & 0xf) << 12)
#define FCR_FUNC_CTRL_CD_DEBOUNCE_TIME_MASK     FCR_FUNC_CTRL_CD_DEBOUNCE_TIME(-1)
#define FCR_FUNC_CTRL_PW_UP_TIME(n)             (((n) & 0xf) << 8)
#define FCR_FUNC_CTRL_PW_UP_TIME_MASK           FCR_FUNC_CTRL_PW_UP_TIME(-1)
#define FCR_FUNC_CTRL_SD_SIG_PULLUP_TIME(n)     (((n) & 0xf) << 4)
#define FCR_FUNC_CTRL_SD_SIG_PULLUP_TIME_MASK   FCR_FUNC_CTRL_SD_SIG_PULLUP_TIME(-1)
#define FCR_FUNC_CTRL_MS_SIG_DELAY(n)           (((n) & 3) << 2)
#define FCR_FUNC_CTRL_MS_SIG_DELAY_MASK         FCR_FUNC_CTRL_MS_SIG_DELAY(-1)
#define FCR_FUNC_CTRL_SD_SIG_DELAY(n)           ((n) & 3)
#define FCR_FUNC_CTRL_SD_SIG_DELAY_MASK         FCR_FUNC_CTRL_SD_SIG_DELAY(-1)

#define FCR_DDRMdEn                             (1 << 19)

#define FCR_CPBLT_VOL_18V                       (1 << 18)
#define FCR_CPBLT_VOL_30V                       (1 << 17)
#define FCR_CPBLT_VOL_33V                       (1 << 16)
#define FCR_CPBLT_SD_BASE_CLK_FREQ(n)           (((n) & 0x3f) << 8)
#define FCR_CPBLT_SD_BASE_CLK_FREQ_MASK         FCR_CPBLT_SD_BASE_CLK_FREQ(-1)
#define FCR_CPBLT_SD_MAX_CURR_CPBLT(n)          ((n) & 0xff)
#define FCR_CPBLT_SD_MAX_CURR_CPBLT_MASK        FCR_CPBLT_SD_MAX_CURR_CPBLT(-1)

#define REG_SDC_BASE                            (0xBC048300L)

#define REG_SDC_DMA_ADDR                        (*(volatile DWORD*)(REG_SDC_BASE+0x00))
#define REG_SDC_BLK_SIZE                        (*(volatile  WORD*)(REG_SDC_BASE+0x04))
#define REG_SDC_BLK_COUNT                       (*(volatile  WORD*)(REG_SDC_BASE+0x06))
#define REG_SDC_ARG                             (*(volatile DWORD*)(REG_SDC_BASE+0x08))
#define REG_SDC_TRAN_MODE                       (*(volatile  WORD*)(REG_SDC_BASE+0x0c))
#define REG_SDC_CMD                             (*(volatile  WORD*)(REG_SDC_BASE+0x0e))
#define REG_SDC_RESP(n)                         (*(volatile DWORD*)(REG_SDC_BASE+0x10+4*((n)&3)))
#define REG_SDC_RESP0                           REG_SDC_RESP(0)
#define REG_SDC_RESP1                           REG_SDC_RESP(1)
#define REG_SDC_RESP2                           REG_SDC_RESP(2)
#define REG_SDC_RESP3                           REG_SDC_RESP(3)
#define REG_SDC_DATA_PORT                       (*(volatile DWORD*)(REG_SDC_BASE+0x20))
#define REG_SDC_STAT                            (*(volatile DWORD*)(REG_SDC_BASE+0x24))
#define REG_SDC_HOST_CTRL                       (*(volatile  BYTE*)(REG_SDC_BASE+0x28))
#define REG_SDC_PW_CTRL                         (*(volatile  BYTE*)(REG_SDC_BASE+0x29))
#define REG_SDC_BLK_GAP_CTRL                    (*(volatile  BYTE*)(REG_SDC_BASE+0x2a))
#define REG_SDC_WAKEUP_CTRL                     (*(volatile  BYTE*)(REG_SDC_BASE+0x2b))
#define REG_SDC_CLK_CTRL                        (*(volatile  WORD*)(REG_SDC_BASE+0x2c))
#define REG_SDC_TIMEOUT_CTRL                    (*(volatile  BYTE*)(REG_SDC_BASE+0x2e))
#define REG_SDC_SW_RESET                        (*(volatile  BYTE*)(REG_SDC_BASE+0x2f))
#define REG_SDC_INT_STAT                        (*(volatile DWORD*)(REG_SDC_BASE+0x30))


#define REG_SDC_INT_STAT_ENABLE                 (*(volatile DWORD*)(REG_SDC_BASE+0x34))
#define REG_SDC_INT_ENABLE                      (*(volatile DWORD*)(REG_SDC_BASE+0x38))

#define SDC_CLK_CTRL_SDCLK_FREQ_SEL(n)          ((((WORD)(n)) & 0xff) << 8)
#define SDC_CLK_CTRL_SDCLK_ENABLE               (1 << 2)
#define SDC_CLK_CTRL_INCLK_STABLE               (1 << 1)
#define SDC_CLK_CTRL_INCLK_ENABLE               (1 << 0)

#define SDC_SW_RESET_ALL                        (1 << 0)
#define SDC_SW_RESET_DAT_LINE                   (1 << 2)
#define SDC_SW_RESET_CMD_LINE                   (1 << 1)

#define SDC_PW_CTRL_BUS_VOL(n)                  ((((WORD)(n)) & 7) << 1)
#define SDC_PW_CTRL_BUS_VOL_33V                 SDC_PW_CTRL_BUS_VOL(7)
#define SDC_PW_CTRL_BUS_VOL_30V                 SDC_PW_CTRL_BUS_VOL(6)
#define SDC_PW_CTRL_BUS_VOL_18V                 SDC_PW_CTRL_BUS_VOL(5)
#define SDC_PW_CTRL_BUS_PW_ON                   (1 << 0)

#define SDC_TIMEOUT_CTRL_DATA_TIMEOUT(n)        ((n) & 0xf)


// Normal Interrupt Status Register (Offset 030h)
#define SDC_INT_ERR_INT                         (1 << 15)  // D09~D14 is Reserved
#define SDC_INT_CARD_INT                        (1 << 8)
#define SDC_INT_CARD_REM                        (1 << 7)
#define SDC_INT_CARD_INS                        (1 << 6)
#define SDC_INT_BUFF_READ_RDY                   (1 << 5)
#define SDC_INT_BUFF_WRITE_RDY                  (1 << 4)
#define SDC_INT_DMA_INT                         (1 << 3)
#define SDC_INT_BLK_GAP_EVENT                   (1 << 2)
#define SDC_INT_TRAN_COMPLETE                   (1 << 1)
#define SDC_INT_CMD_COMPLETE                    (1 << 0)


//#define KEEP_ORIGINAL_eMMC_READ // without conceal memory copy time inside eMMC device read time.

#ifndef KEEP_ORIGINAL_eMMC_READ

// **********************************************************************************
// Note : "#ifndef KEEP_ORIGINAL_eMMC_READ" is true, then it needs to open one of the following 3 #define, ***
// #define eMMC_DMA_MEMCOPY_USE_INT_FOR_READ                                                                                         ***
// #define eMMC_DMA_MEMCOPY_USE_POLLING                                                                                  ***
// #define eMMC_REMOVE_DMA_MEMCOPY                                                                                         ***
// **********************************************************************************

//#define eMMC_DMA_MEMCOPY_USE_INT_FOR_READ // Conceal memory copy time inside eMMC device read time during DMA boundary interrupt tasklet.
#ifdef eMMC_DMA_MEMCOPY_USE_INT_FOR_READ

//SD HOST controller : Block Size Register (Offset 004h)
//[D14:D12] Host DMA Buffer Bound
//000b 4K bytes (Detects A11 carry out)
//001b 8K bytes (Detects A12 carry out)
//010b 16K Bytes (Detects A13 carry out)
//011b 32K Bytes (Detects A14 carry out)
//100b 64K bytes (Detects A15 carry out)
//101b 128K Bytes (Detects A16 carry out)
//110b 256K Bytes (Detects A17 carry out)
//111b 512K Bytes (Detects A18 carry out)
//#define eMMC_DMA_BUFFER_BOUND_SETTING_For_READ		4	//100b 64K bytes (Detects A15 carry out)
#define eMMC_DMA_BUFFER_BOUND_SETTING_For_READ		3	//011b 32K Bytes (Detects A14 carry out)
//#define eMMC_DMA_BUFFER_BOUND_SETTING_For_READ		2	//010b 16K Bytes (Detects A13 carry out)
//#define eMMC_DMA_BUFFER_BOUND_SETTING_For_READ		7	//111b 512K Bytes (Detects A18 carry out)
#define eMMC_DMA_BUFFER_BOUND_SIZE_For_READ			(1<<(eMMC_DMA_BUFFER_BOUND_SETTING_For_READ+12))

#endif


//#define eMMC_DMA_MEMCOPY_USE_POLLING // Conceal memory copy time inside eMMC device read time by polling.
#ifdef eMMC_DMA_MEMCOPY_USE_POLLING
#define eMMC_DMA_MEMCOPY_SIZE						16*1024 //64*1024 // 32*1024
#endif // eMMC_DMA_MEMCOPY_USE_POLLING


//Ben, open the following "#define eMMC_REMOVE_DMA_MEMCOPY" will do parsing scatterlist buffer and eliminate the need of using bounce buffer.
#define eMMC_REMOVE_DMA_MEMCOPY


#endif // #ifndef KEEP_ORIGINAL_eMMC_READ

#define NT72558_Cache_Line_Size 64

//#define NT72558_eMMC_DEBUG // open this define to use GPA4 as debug signal.


//#define eMMC_TEST_WRIE_WITH_READ_BACK_CHECK
#ifdef eMMC_TEST_WRIE_WITH_READ_BACK_CHECK

int SDC_ReadSector_Internal(char* dwBuffer, DWORD dwSectorAddress, DWORD dwSectorCount, DWORD dwSectorSize);
bool DRV_SDC_CardStatus_Inserted(void);
DWORD SDC_CMD13(void);

// REG_SDC_STAT
#define SDC_STAT_CMD_SIG                        (1 << 24)
#define SDC_STAT_DAT_SIG(n)                     (1 << (20 + (n)))
#define SDC_STAT_DAT_SIG0                       SDC_STAT_DAT_SIG(0)
#define SDC_STAT_DAT_SIG1                       SDC_STAT_DAT_SIG(1)
#define SDC_STAT_DAT_SIG2                       SDC_STAT_DAT_SIG(2)
#define SDC_STAT_DAT_SIG3                       SDC_STAT_DAT_SIG(3)
#define SDC_STAT_WP_PIN                         (1 << 19)
#define SDC_STAT_CD_PIN                         (1 << 18)
#define SDC_STAT_CARD_STAT_STABLE               (1 << 17)
#define SDC_STAT_CARD_INS                       (1 << 16)
#define SDC_STAT_BUFF_READ_ENABLE               (1 << 11)
#define SDC_STAT_BUFF_WRITE_ENABLE              (1 << 10)
#define SDC_STAT_READ_TRAN_ACTIVE               (1 << 9)
#define SDC_STAT_WRITE_TRAN_ACTIVE              (1 << 8)
#define SDC_STAT_DAT_LINE_ACTIVE                (1 << 2)
#define SDC_STAT_CMD_INHIBIT_DAT                (1 << 1)
#define SDC_STAT_CMD_INHIBIT_CMD                (1 << 0)

#define RES_TYPE_SUCCESS	0
#define RES_TYPE_ERR		1
#define RES_TIMEOUT_ERR	             -19

// REG_SDC_BLK_SIZE
#define SDC_BLK_SIZE_DMA_BUFF_BND(n)            ((WORD)((n) & 7) << 12)
#define SDC_BLK_SIZE_DMA_BUFF_BND_MASK          SDC_BLK_SIZE_DMA_BUFF_BND(-1)
#define SDC_BLK_SIZE_DMA_BUFF_BND_512K          SDC_BLK_SIZE_DMA_BUFF_BND(7)
#define SDC_BLK_SIZE_DMA_BUFF_BND_256K          SDC_BLK_SIZE_DMA_BUFF_BND(6)
#define SDC_BLK_SIZE_DMA_BUFF_BND_128K          SDC_BLK_SIZE_DMA_BUFF_BND(5)
#define SDC_BLK_SIZE_DMA_BUFF_BND_64K           SDC_BLK_SIZE_DMA_BUFF_BND(4)
#define SDC_BLK_SIZE_DMA_BUFF_BND_32K           SDC_BLK_SIZE_DMA_BUFF_BND(3)
#define SDC_BLK_SIZE_DMA_BUFF_BND_16K           SDC_BLK_SIZE_DMA_BUFF_BND(2)
#define SDC_BLK_SIZE_DMA_BUFF_BND_8K            SDC_BLK_SIZE_DMA_BUFF_BND(1)
#define SDC_BLK_SIZE_DMA_BUFF_BND_4K            SDC_BLK_SIZE_DMA_BUFF_BND(0)
#define SDC_BLK_SIZE_TRAN_BLK_SIZE(n)           ((WORD)((n) & 0xfff))

// -------------------------------------------------------------------------------------------------
// REG_SDC_TRAN_MODE
#define SDC_TRAN_MODE_MULT_BLK                  (1 << 5)
#define SDC_TRAN_MODE_READ                      (1 << 4)
#define SDC_TRAN_MODE_AUTO_CMD12                (1 << 2)
#define SDC_TRAN_MODE_BLK_COUNT                 (1 << 1)
#define SDC_TRAN_MODE_DMA                       (1 << 0)

// REG_SDC_CMD
#define SDC_CMD_IDX(n)                          ((((WORD)(n)) & 0x3f) << 8)
#define SDC_CMD_TYPE_ABORT                      (3 << 6)
#define SDC_CMD_TYPE_RESUME                     (2 << 6)
#define SDC_CMD_TYPE_SUSPEND                    (1 << 6)
#define SDC_CMD_TYPE_NORMAL                     (0 << 6)
#define SDC_CMD_DATA_PRESENT                    (1 << 5)
#define SDC_CMD_IDX_CHK                         (1 << 4)
#define SDC_CMD_CRC_CHK                         (1 << 3)
#define SDC_CMD_RESP_TYPE_LEN_0                 (0 << 0)
#define SDC_CMD_RESP_TYPE_LEN_136               (1 << 0)
#define SDC_CMD_RESP_TYPE_LEN_48                (2 << 0)
#define SDC_CMD_RESP_TYPE_LEN_48_BUSY_CHK       (3 << 0)

#endif // #ifdef eMMC_TEST_WRIE_WITH_READ_BACK_CHECK

#endif // #ifndef __NT72558_MMC_H__

