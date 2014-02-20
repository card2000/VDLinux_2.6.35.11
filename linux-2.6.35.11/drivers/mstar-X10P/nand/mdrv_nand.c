#include <linux/module.h>
#include <linux/string.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>

#include <linux/semaphore.h>

#include "include/drvNand.h"

U32 u32CurRow = 0;
U32 u32CurCol = 0;
extern U32 u32CIFDIdx;
extern U16 u16ByteIdxofPage;
extern U32 u32WriteLen;

//#define NAND_DEBUG(fmt, args...)	printk(fmt, ##args)
#define NAND_DEBUG(fmt, args...)

extern struct semaphore				PfModeSem;
#define PF_MODE_SEM(x)				//(x)

/* These really don't belong here, as they are specific to the NAND Model */
static uint8_t scan_ff_pattern[] = { 0xFF};

/* struct nand_bbt_descr - bad block table descriptor */
static struct nand_bbt_descr _titania_nand_bbt_descr =
{
	.options =	NAND_BBT_SCANALLPAGES,//NAND_BBT_2BIT | NAND_BBT_LASTBLOCK | NAND_BBT_VERSION | NAND_BBT_CREATE | NAND_BBT_WRITE,
	.offs =		0,
	.len =		1,
	.pattern =	scan_ff_pattern
};

static struct nand_ecclayout titania_nand_oob_16 =
{
	.eccbytes =	10,
	.eccpos =	{6,7,8,9,10,11,12,13,14,15},
	.oobfree = 
	{
		{
			.offset = 0,
			.length = 5
		}
	}
};

static struct nand_ecclayout titania_nand_oob_64 =
{
    .eccbytes =	40,
    .eccpos = 
    {
		0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
		0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,
		0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,
		0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F
    },
    .oobfree =
    {
		{
			.offset = 1,
			.length = 5
		},
		{
			.offset = 0x10,
			.length = 6
		},
		{
			.offset = 0x20,
			.length = 6
		},
		{
			.offset = 0x30,
			.length = 6
		}
    }
};

static struct nand_ecclayout titania_nand_oob_432 =
{
    .eccbytes =	336,
    .eccpos = 
    {
    	// Sector 0
		0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12,
		0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
		0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20,
		0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
		0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E,
		0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,

		// Secotr 1
		0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
		0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
		0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56,
		0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D,
		0x5E, 0x5F, 0x60, 0x61, 0x62, 0x63, 0x64,
		0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6b,

		// Sector 2
		0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E,
		0x7F, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85,
		0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C,
		0x8D, 0x8E, 0x8F, 0x90, 0x91, 0x92, 0x93,
		0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A,
		0x9B, 0x9C, 0x9D, 0x9E, 0x9F, 0xA0, 0xA1,

		// Sector 3
		0xAE, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3, 0xB4,
		0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB,
		0xBC, 0xBD, 0xBE, 0xBF, 0xC0, 0xC1, 0xC2,
		0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9,
		0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 0xD0,
		0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7,
		
		// Secotr 4
		0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA,
		0xEB, 0xEC, 0xED, 0xEE, 0xEF, 0xF0, 0xF1,
		0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8,
		0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF,
		0x100, 0x101, 0x102, 0x103, 0x104, 0x105, 0x106,
		0x107, 0x108, 0x109, 0x10A, 0X10B, 0x10C, 0x10D,

		// Sector 5
		0x11A, 0x11B, 0x11C, 0x11D, 0x11E, 0x11F, 0x120,
		0x121, 0x122, 0x123, 0x124, 0X125, 0x126, 0x127,
		0x128, 0x129, 0x12A, 0x12B, 0x12C, 0x12D, 0x12E,
		0x12F, 0x130, 0x131, 0x132, 0X133, 0x134, 0x135,
		0x136, 0x137, 0x138, 0x139, 0x13A, 0x13B, 0x13C,
		0x13D, 0x13E, 0x13F, 0x140, 0X141, 0x142, 0x143,

		// Sector 6
		0x150, 0x151, 0x152, 0x153, 0x154, 0x155, 0x156,
		0x157, 0x158, 0x159, 0x15A, 0X15B, 0x15C, 0x15D,
		0x15E, 0x15F, 0x160, 0x161, 0x162, 0x163, 0x164,
		0x165, 0x166, 0x167, 0x168, 0X169, 0x16A, 0x16B,
		0x16C, 0x16D, 0x16E, 0x16F, 0x170, 0x171, 0x172,
		0x173, 0x174, 0x175, 0x176, 0X177, 0x178, 0x179,

		// Sector 7
		0x186, 0x187, 0x188, 0x189, 0x18A, 0x18B, 0x18C,
		0x18D, 0x18E, 0x18F, 0x190, 0X191, 0x192, 0x193,
		0x194, 0x195, 0x196, 0x197, 0x198, 0x199, 0x19A,
		0x19B, 0x19C, 0x19D, 0x19E, 0X19F, 0x1A0, 0x1A1,
		0x1A2, 0x1A3, 0x1A4, 0x1A5, 0x1A6, 0x1A7, 0x1A8,
		0x1A9, 0x1AA, 0x1AB, 0x1AC, 0X1AD, 0x1AE, 0x1AF,
    },
    .oobfree =
    {
		{
			.offset = 1,
			.length = 0xB
		},
		{
			.offset = 0x36,
			.length = 0xC
		},
		{
			.offset = 0x6C,
			.length = 0xC
		},
		{
			.offset = 0xA2,
			.length = 0xC
		},
		{
			.offset = 0xD8,
			.length = 0xC
		},
		{
			.offset = 0x10E,
			.length = 0xC
		},
		{
			.offset = 0x144,
			.length = 0xC
		},
		{
			.offset = 0x17A,
			.length = 0xC
		}
    }
};


#if 1
static uint8_t bbt_pattern[] = {'B', 'b', 't', '0' };
static uint8_t mirror_pattern[] = {'1', 't', 'b', 'B' };

static struct nand_bbt_descr _titania_bbt_main_descr = {
	.options =		NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE |
					NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs =			1,
	.len =			3,
	.veroffs =		4,
	.maxblocks =	4,
	.pattern =		bbt_pattern
};

static struct nand_bbt_descr _titania_bbt_mirror_descr = {
	.options =		NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE |
					NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs =			1,
	.len =			3,
	.veroffs =		4,
	.maxblocks =	4,
	.pattern =		mirror_pattern
};
#else
static uint8_t bbt_pattern[] = {0xff};
static uint8_t mirror_pattern[] = {0xff};

static struct nand_bbt_descr _titania_bbt_main_descr = {
	.options =		NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE |
					NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs =			1,
	.len =			3,
	.veroffs =		4,
	.maxblocks =	4,
	.pattern =		bbt_pattern
};

static struct nand_bbt_descr _titania_bbt_mirror_descr = {
	.options =		NAND_BBT_SCAN2NDPAGE,
	.offs =			0,
	.len =			1,
	.veroffs =		4,
	.maxblocks =	4,
	.pattern =		mirror_pattern
};

#endif


#ifdef CONFIG_MTD_CMDLINE_PARTS
extern int parse_cmdline_partitions(struct mtd_info *master, struct mtd_partition **pparts, char *);
#endif

#define MSTAR_NAND_SIZE						0x4000000

#define SZ_256KB							(256 * 1024)
#define SZ_512KB							(512 * 1024)
#define SZ_2MB								(2 * 1024 * 1024)
#define SZ_4MB								(4 * 1024 * 1024)
#define SZ_7MB								(7 * 1024 * 1024)
#define SZ_8MB								(8 * 1024 * 1024)
#define SZ_16MB								(16 * 1024 * 1024)
#define SZ_32MB								(32 * 1024 * 1024)

#define NAND_PART_PARTITION_TBL_OFFSET		0
#define NAND_PART_PARTITION_TBL_SIZE		SZ_256KB

#define NAND_PART_LINUX_BOOT_PARAM_OFFSET	(NAND_PART_PARTITION_TBL_OFFSET + NAND_PART_PARTITION_TBL_SIZE)
#define NAND_PART_LINUX_BOOT_PARAM_SIZE		SZ_256KB

#define NAND_PART_KERNEL_OFFSET				(NAND_PART_LINUX_BOOT_PARAM_OFFSET + NAND_PART_LINUX_BOOT_PARAM_SIZE)
#define NAND_PART_KERNEL_SIZE				SZ_4MB

#define NAND_PART_CHAKRA_BOOT_PARAM_OFFSET	(NAND_PART_KERNEL_OFFSET + NAND_PART_KERNEL_SIZE)
#define NAND_PART_CHAKRA_BOOT_PARAM_SIZE	SZ_512KB

#define NAND_PART_CHAKRA_BIN_OFFSET			(NAND_PART_CHAKRA_BOOT_PARAM_OFFSET + NAND_PART_CHAKRA_BOOT_PARAM_SIZE)
#define NAND_PART_CHAKRA_BIN_PARAM_SIZE		SZ_8MB

#define NAND_PART_ROOTFS_OFFSET				(NAND_PART_CHAKRA_BIN_OFFSET + NAND_PART_CHAKRA_BIN_PARAM_SIZE)
#define NAND_PART_ROOTFS_SIZE				SZ_4MB

#define NAND_PART_CONF_OFFSET				(NAND_PART_ROOTFS_OFFSET + NAND_PART_ROOTFS_SIZE)
#define NAND_PART_CONF_SIZE					SZ_256KB

#define NAND_PART_SUBSYSTEM_OFFSET			(NAND_PART_CONF_OFFSET + NAND_PART_CONF_SIZE)
#define NAND_PART_SUBSYSTEM_SIZE			SZ_2MB

#define NAND_PART_FONT_OFFSET				(NAND_PART_SUBSYSTEM_OFFSET + NAND_PART_SUBSYSTEM_SIZE)
#define NAND_PART_FONT_SIZE					SZ_4MB

#define NAND_PART_OPT_OFFSET				(NAND_PART_FONT_OFFSET + NAND_PART_FONT_SIZE)
#define NAND_PART_OPT_SIZE					SZ_8MB

#define NAND_PART_APPLICATION_OFFSET		(NAND_PART_OPT_OFFSET + NAND_PART_OPT_SIZE)
#define NAND_PART_APPLICATION_SIZE			(MSTAR_NAND_SIZE - NAND_PART_APPLICATION_OFFSET)

#if ( (NAND_PART_APPLICATION_OFFSET) >= MSTAR_NAND_SIZE)
    #error "Error: NAND partition is not correct!!!"
#endif

static const struct mtd_partition partition_info[] =
{
#if 1
#if 0
	{
		.name   = "ecc_test",
		.offset = 0x0,
		.size   = 0x6400000
	},
	{
		.name   = "application",
		.offset = 0x6400000,
		.size   = 0x79C00000
	},
#else
	{
		.name   = "application",
		.offset = 0x0,
		.size   = 0x80000000
	},

#endif
#else
	{
		.name   = "ubitest",
		.offset = 0x0,
		.size   = 0x1E00000
	},

	{
		.name   = "application",
		.offset = 0x1E00000,
		.size   = 0x7E200000
	},
#endif
};

#define NUM_PARTITIONS ARRAY_SIZE(partition_info)

static struct mtd_info *nand_mtd = NULL;

static void _titania_nand_hwcontrol(struct mtd_info *mtdinfo, int cmd, unsigned int ctrl)
{
    NAND_DEBUG("NANDDrv_HWCONTROL()\n");

    if(ctrl & NAND_CTRL_CHANGE)
    {
		if(ctrl & NAND_NCE)
		{
			NAND_DEBUG("NAND_CTL_SETNCE\n");
			PF_MODE_SEM(down(&PfModeSem));
			drvNAND_SetCEZ(0x00);
			PF_MODE_SEM(up(&PfModeSem));
		}
		else
		{
			NAND_DEBUG("NAND_CTL_CLRNCE\n");
			PF_MODE_SEM(down(&PfModeSem));
			drvNAND_SetCEZ(0x1);
			PF_MODE_SEM(up(&PfModeSem));
		}
        
		if(ctrl & NAND_CLE)
        {
			NAND_DEBUG("NAND_CTL_SETCLE\n");
			PF_MODE_SEM(down(&PfModeSem));
			drvNAND_SetCLE(0x01);
			PF_MODE_SEM(up(&PfModeSem));
		}
		else
		{
			NAND_DEBUG("NAND_CTL_CLRCLE\n");
			PF_MODE_SEM(down(&PfModeSem));
			drvNAND_SetCLE(0x0);
			PF_MODE_SEM(up(&PfModeSem));
		}
		
		if(ctrl & NAND_ALE)
		{
			NAND_DEBUG("NAND_CTL_SETALE\n");
			PF_MODE_SEM(down(&PfModeSem));
			drvNAND_SetALE(0x1);
			PF_MODE_SEM(up(&PfModeSem));
		}
		else
		{
			NAND_DEBUG("NAND_CTL_CLRALE\n");
			PF_MODE_SEM(down(&PfModeSem));
			drvNAND_SetALE(0x0);
			PF_MODE_SEM(up(&PfModeSem));
		}
	}
}

static int _titania_nand_device_ready(struct mtd_info *mtdinfo)
{
	NAND_DEBUG("NANDDrv_DEVICE_READY()\n");

	PF_MODE_SEM(down(&PfModeSem));
	drvNAND_WaitReady(1000);
	PF_MODE_SEM(up(&PfModeSem));
	
	return 1;
}

static void _titania_nand_write_buf(struct mtd_info *mtd, const u_char *buf, int len)
{
    NAND_DEBUG("NANDDrv_WRITE_BUF() 0x%X\r\n",len);

	PF_MODE_SEM(down(&PfModeSem));
    drvNAND_WriteBuf((U8 *const)buf, len);
	PF_MODE_SEM(up(&PfModeSem));
}

static void _titania_nand_read_buf(struct mtd_info *mtd, u_char* const buf, int len)
{
    NAND_DEBUG("NANDDrv_READ_BUF()0x%X\n",len);
    
	PF_MODE_SEM(down(&PfModeSem));
    drvNAND_ReadBuf(buf, len);
	PF_MODE_SEM(up(&PfModeSem));
}

static u16 _titania_nand_read_word(struct mtd_info *mtd)
{
    NAND_DEBUG("NANDDrv_READ_WORD()\n");
    return 0;
}

static u_char _titania_nand_read_byte(struct mtd_info *mtd)
{
    U8 u8Ret;
    
    NAND_DEBUG("NANDDrv_READ_BYTE()\n");
    
    PF_MODE_SEM(down(&PfModeSem));
	u8Ret = drvNAND_ReadByte();
    PF_MODE_SEM(up(&PfModeSem));
    
    return (u8Ret);
}

/**
 * nand_wait - [DEFAULT]  wait until the command is done
 * @mtd:	MTD device structure
 * @this:	NAND chip structure
 * @state:	state to select the max. timeout value
 *
 * Wait for command done. This applies to erase and program only
 * Erase can take up to 400ms and program up to 20ms according to
 * general NAND and SmartMedia specs
 *
*/

static int _titania_nand_wait(struct mtd_info *mtd, struct nand_chip *this)
{
	U8 u8Tmp;
	
    NAND_DEBUG("NANDDrv_WAIT()\n");

	PF_MODE_SEM(down(&PfModeSem));
	drvNAND_WaitReady(1000);
	drvNand_Read_Status();
	NC_GetCIFD(&u8Tmp, 0, 1);
	PF_MODE_SEM(up(&PfModeSem));
	return u8Tmp;
}

static void _titania_nand_cmdfunc(struct mtd_info *mtd, unsigned command,
                        int column, int page_addr)
{
    PF_MODE_SEM(down(&PfModeSem));
    
	#if defined(NAND_CE_GPIO) && NAND_CE_GPIO
	drvNAND_SetCEGPIO(0);
	#endif
	#if defined(NAND_WP_GPIO) && NAND_WP_GPIO
	REG_SET_BITS_UINT16(ONIF_CONFIG0, R_PIN_RPZ);
	//drvNAND_SetWPGPIO(1);
	#endif
    REG_CLR_BITS_UINT16(NC_SIGNAL, BIT_NC_WP_AUTO | BIT_NC_CE_AUTO | BIT_NC_CE_H | BIT_NC_CE_SEL_MASK);
    REG_SET_BITS_UINT16(NC_SIGNAL, BIT_NC_CHK_RB_HIGH | BIT_NC_WP_H);
    
	switch (command) {
        case NAND_CMD_READ0:
            NAND_DEBUG("_titania_nand_cmdfunc: NAND_CMD_READ0, page_addr: 0x%x, column: 0x%x.\n", page_addr, (column>>1));
            u32CurRow = page_addr;
            u32CurCol = column;
            break;

        case NAND_CMD_READ1:
            NAND_DEBUG("_titania_nand_cmdfunc: NAND_CMD_READ1.\n");
            u32CurRow = page_addr;
            u32CurCol = column;
            break;

        case NAND_CMD_READOOB:
            NAND_DEBUG("_titania_nand_cmdfunc: NAND_CMD_READOOB.\n");
			drvNAND_CmdReadOOB(page_addr, column);
            break;

        case NAND_CMD_READID:
            NAND_DEBUG("_titania_nand_cmdfunc: NAND_CMD_READID.\n");
            u32CIFDIdx = 0;
			NC_ReadID();
            break;

        case NAND_CMD_PAGEPROG:
            /* sent as a multicommand in NAND_CMD_SEQIN */
            NAND_DEBUG("_titania_nand_cmdfunc: NAND_CMD_PAGEPROG.\n");
            u32CurRow = page_addr;
            u32CurCol = column;
            break;

        case NAND_CMD_ERASE1:
            NAND_DEBUG("_titania_nand_cmdfunc: NAND_CMD_ERASE1,  page_addr: 0x%x, column: 0x%x.\n", page_addr, column);
			NC_EraseBlk(page_addr);
            break;

        case NAND_CMD_ERASE2:
            NAND_DEBUG("_titania_nand_cmdfunc: NAND_CMD_ERASE2.\n");
            break;

        case NAND_CMD_SEQIN:
            /* send PAGE_PROG command(0x1080) */
            NAND_DEBUG("_titania_nand_cmdfunc: NAND_CMD_SEQIN/PAGE_PROG,  page_addr: 0x%x, column: 0x%x.\n", page_addr, column);
			u32CurRow = page_addr;
            u32CurCol = column;
			u16ByteIdxofPage = column;
			u32WriteLen = 0;
            break;

        case NAND_CMD_STATUS:
            NAND_DEBUG("_titania_nand_cmdfunc: NAND_CMD_STATUS.\n");
            u32CIFDIdx = 0;
            drvNand_Read_Status();
            break;

        case NAND_CMD_RESET:
        	NAND_DEBUG("_titania_nand_cmdfunc: NAND_CMD_RESET.\n");
			NC_ResetNandFlash();
            break;

        case NAND_CMD_STATUS_MULTI:
            NAND_DEBUG("_titania_nand_cmdfunc: NAND_CMD_STATUS_MULTI.\n");
            u32CIFDIdx = 0;
			drvNand_Read_Status();
            break;

        case NAND_CMD_READSTART:
            NAND_DEBUG("_titania_nand_cmdfunc: NAND_CMD_READSTART.\n");
            break;

        case NAND_CMD_CACHEDPROG:
            NAND_DEBUG("_titania_nand_cmdfunc: NAND_CMD_CACHEDPROG.\n");
            break;

        default:
            printk("_titania_nand_cmdfunc: error, unsupported command.\n");
            break;
	}
	
    PF_MODE_SEM(up(&PfModeSem));
}

static void _titania_nand_enable_hwecc(struct mtd_info *mtd, int mode)
{
    NAND_DEBUG("enable_hwecc\r\n");
    // default enable
}

static int _titania_nand_calculate_ecc(struct mtd_info *mtd, const u_char *dat, u_char *ecc_code)
{    
    NAND_DEBUG("calculate_ecc\r\n");
	return 0;
}

static int _titania_nand_correct_data(struct mtd_info *mtd, u_char *dat, u_char *read_ecc, u_char *calc_ecc)
{
    NAND_DEBUG("correct_data\r\n");
	return drvNand_CheckECC();
}

/*
 * Board-specific NAND initialization.
 * - hwcontrol: hardwarespecific function for accesing control-lines
 * - dev_ready: hardwarespecific function for  accesing device ready/busy line
 * - eccmode: mode of ecc, see defines
 */
static int __init mstar_nand_init(void)
{
	struct nand_chip *nand;
	int err = 0;	
	NAND_DRIVER *pNandDrv = (NAND_DRIVER*)drvNand_get_DrvContext_address();

	/* Allocate memory for MTD device structure and private data */
	nand_mtd = kmalloc (sizeof(struct mtd_info) + sizeof (struct nand_chip), GFP_KERNEL);
	if (!nand_mtd) {
		printk ("Unable to allocate NAND MTD device structure.\n");
		return -ENOMEM;
	}

	/* Get pointer to private data */
	nand = (struct nand_chip *) (&nand_mtd[1]);

	/* Initialize structures */
	memset((char *) nand_mtd, 0, sizeof(struct mtd_info));
	memset((char *) nand, 0, sizeof(struct nand_chip));

	nand_mtd->priv = nand;

    NAND_DEBUG("NANDDrv_BOARD_NAND_INIT().\n");

    PF_MODE_SEM(down(&PfModeSem));
    
	#if defined(NAND_CE_GPIO) && NAND_CE_GPIO
	drvNAND_SetCEGPIO(0);
	#endif
	#if defined(NAND_WP_GPIO) && NAND_WP_GPIO
	REG_SET_BITS_UINT16(ONIF_CONFIG0, R_PIN_RPZ);
	//drvNAND_SetWPGPIO(1);
	#endif
    REG_CLR_BITS_UINT16(NC_SIGNAL, BIT_NC_WP_AUTO | BIT_NC_CE_AUTO | BIT_NC_CE_H | BIT_NC_CE_SEL_MASK);
    REG_SET_BITS_UINT16(NC_SIGNAL, BIT_NC_CHK_RB_HIGH | BIT_NC_WP_H);
	drvNAND_Init();
    
    PF_MODE_SEM(up(&PfModeSem));

    /* please refer to include/linux/nand.h for more info. */

    nand->dev_ready						= _titania_nand_device_ready;
    nand->cmd_ctrl						= _titania_nand_hwcontrol;
    nand->ecc.mode						= NAND_ECC_HW;

	if( pNandDrv->u32_PageByteCnt == 8192 )
    {
        nand->ecc.size					= 8192;
        nand->ecc.bytes					= 336;
        nand->ecc.layout				= &titania_nand_oob_432;
        _titania_nand_bbt_descr.offs	= 0;
    }
    else
    {
		nand->ecc.size					= 512;
		nand->ecc.bytes					= 10;
		nand->ecc.layout				= &titania_nand_oob_16;
    }
    
    nand->ecc.hwctl						= _titania_nand_enable_hwecc;
    nand->ecc.correct					= _titania_nand_correct_data;
    nand->ecc.calculate					= _titania_nand_calculate_ecc;
    
	if( pNandDrv->u32_PageByteCnt == 8192 )
		nand->options					= 0;//NAND_USE_FLASH_BBT |  NAND_NO_AUTOINCR;
	else
		nand->options					= NAND_USE_FLASH_BBT;
	
    nand->waitfunc						= _titania_nand_wait;
    nand->read_byte						= _titania_nand_read_byte;
    nand->read_word						= _titania_nand_read_word;
    nand->read_buf						= _titania_nand_read_buf;
    nand->write_buf						= _titania_nand_write_buf;
    nand->chip_delay					= 20;
    nand->cmdfunc						= _titania_nand_cmdfunc;
    nand->badblock_pattern				= &_titania_nand_bbt_descr; //using default badblock pattern.
 //   nand->bbt_td						= &_titania_bbt_main_descr;
//    nand->bbt_md						= &_titania_bbt_mirror_descr;
    
	#if defined(NAND_CE_GPIO) && NAND_CE_GPIO
	drvNAND_SetCEGPIO(0);
	#endif
	#if defined(NAND_WP_GPIO) && NAND_WP_GPIO
	REG_SET_BITS_UINT16(ONIF_CONFIG0, R_PIN_RPZ);
	//drvNAND_SetWPGPIO(1);
	#endif
	REG_CLR_BITS_UINT16(NC_SIGNAL, BIT_NC_WP_AUTO | BIT_NC_CE_AUTO | BIT_NC_CE_H | BIT_NC_CE_SEL_MASK);
    REG_SET_BITS_UINT16(NC_SIGNAL, BIT_NC_CHK_RB_HIGH | BIT_NC_WP_H);
	
	if ((err = nand_scan(nand_mtd, 1)) != 0) {
		printk("can't register NAND\n");
		return -ENOMEM;
	}

#ifdef CONFIG_MTD_CMDLINE_PARTS
    {
        int mtd_parts_nb = 0;
        struct mtd_partition *mtd_parts = 0;
        mtd_parts_nb = parse_cmdline_partitions(nand_mtd, &mtd_parts, "samsung-nand");
        if (mtd_parts_nb > 0) {
            add_mtd_partitions(nand_mtd, mtd_parts, mtd_parts_nb);
            printk("parse_cmdline_partitions: add_mtd_partitions\n");
        }
        else {
            #if 0
            add_mtd_device(nand_mtd);
            printk("parse_cmdline_partitions: add_mtd_device\n");
            #else
            add_mtd_partitions(nand_mtd, partition_info, NUM_PARTITIONS); // use default partition table if parsing cmdline partition fail
            #endif
        }
    }
#else
	add_mtd_partitions(nand_mtd, partition_info, NUM_PARTITIONS);
#endif
    return 0;
}

static void __exit mstar_nand_cleanup(void)
{
}

module_init(mstar_nand_init);
module_exit(mstar_nand_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("MStar");
MODULE_DESCRIPTION("MStar MTD NAND driver");
