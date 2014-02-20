
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
//#define SDCLK_SOURCE    (10000000)

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
