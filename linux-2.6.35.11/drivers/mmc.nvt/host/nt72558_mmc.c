/*
 * Use SDHCI and its platform driver for NVT72558 eMMC host which is
 * SDHC standard specification version 1.0 compatible.
 */
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/mmc/host.h>
#include <linux/io.h>
#include <asm/mips-boards/nvt72558.h>
#include <asm/mips-boards/nvt72558_int.h>
#include <linux/sdhci-pltfm.h>
#include <linux/delay.h>

#include "nt72558_mmc.h"
#include "sdhci.h"

#define SDC_CLK2DIV(CLK) (((SDCLK_SOURCE) + (CLK) - 1) / (CLK))  // Maximum CLK = 52000000 = 52MHz
static DWORD DRV_SDC_InitClock(DWORD dwDivisor)
{
	REG_SDC_CLK_CTRL = 0;   // turn off clock
        
	
	if(dwDivisor > 0 && dwDivisor <= 1024)
	{   // range of valid divisor (1~1024)
		if(dwDivisor <= (257*2))
		{
			// divisor = 2, 3, ..., 257  ->  flexible mode
			//Ben, for 72688/72558 FCR, March/07/2011.
			//SDCLK = SDCLK_Source / (( REG_SDC_CLK_CTRL[15:8] + 2 ) *2)
			//SDCLK_Source = 52*4
			//#define SDC_CLK2DIV(CLK) (((CPU_SPEED) + (CLK) - 1) / (CLK)) --> for 72688/72558, #define SDC_CLK2DIV(CLK) (((SDCLK_Source) + (CLK) - 1) / (CLK))
			//dwDivisor = ( REG_SDC_CLK_CTRL[15:8] + 2 ) *2
			//REG_SDC_CLK_CTRL[15:8] = dwDivisor/2 - 2 while dwDivisor >= 4, maximum SDCLK = SDCLK_Source/4 = 52*4/4 = 52MHz
        		
			//Ben+, March/22/2011.
			REG_FCR_FUNC_CTRL |= FCR_FUNC_CTRL_SD_FLEXIBLE_CLK; // use flexible SD clock frequency selection
			dwDivisor = dwDivisor < 4 ? 4 : dwDivisor;
			REG_SDC_CLK_CTRL = SDC_CLK_CTRL_INCLK_ENABLE | SDC_CLK_CTRL_SDCLK_FREQ_SEL(dwDivisor/2 - 2);
			//REG_SDC_CLK_CTRL = SDC_CLK_CTRL_INCLK_ENABLE | 0x100; // --> Ben, note, 52*4MHz divide by 6.
		}
		else
		{
			//Original
			// divisor = 258, 259, ..., 1024  ->  standard mode
			//dwDivisor = dwDivisor <= 512 ? 512 : 1024;
			//Ben, for 72688/72558, divisor = 257*2+1, 257*2+2, ..., 1024
			dwDivisor = 1024;
			//Ben, for 72688/72558 FCR, March/07/2011.
			//SDCLK_Source = 52*4
			//SDCLK = SDCLK_Source / 4* REG_SDC_CLK_CTRL[15:8] * 2, while MSB of REG_SDC_CLK_CTRL[15:8] = 0, 1, 2, 4, 8, 16, 32, 64, 128.
			//dwDivisor = 4* REG_SDC_CLK_CTRL[15:8] * 2 --> REG_SDC_CLK_CTRL[15:8] = dwDivisor/8;
			REG_FCR_FUNC_CTRL &= ~FCR_FUNC_CTRL_SD_FLEXIBLE_CLK;    // use standard SD clock frequency selection
			REG_SDC_CLK_CTRL = SDC_CLK_CTRL_INCLK_ENABLE | SDC_CLK_CTRL_SDCLK_FREQ_SEL(dwDivisor / 8);  // set internal clock
		}

		while(!(REG_SDC_CLK_CTRL & SDC_CLK_CTRL_INCLK_STABLE)); // wait for internal clock stable
			REG_SDC_CLK_CTRL |= SDC_CLK_CTRL_SDCLK_ENABLE;  // sdclk on
	
		mdelay(2);    // delay 2ms for output clock stable
	}		
	else
	{
		// invalid range of divisor (0 or >1024)
		dwDivisor = 0;
		//Ben, debug, Apr/18/2011.
		printk("DRV_SDC_InitClock : invalid range of clock divisor!\n");
	}

	return dwDivisor;
}

static bool DRV_SDC_SysInit (void)
{
	REG_FCR_FUNC_CTRL = 0x003f3020; // 16/8/4 Beats Transfer is allowed.

	//Ben, For eMMC usage, neeed to set SD CD flag by software, March/04/2011.
	// --> Need to set software SD CD Mode
	REG_FCR_FUNC_CTRL |= FCR_FUNC_CTRL_SW_CDWP_ENABLE;  // Enable Software SD CD
	// --> Force card detection is TRUE
	REG_FCR_FUNC_CTRL &= ~FCR_FUNC_CTRL_SW_SD_CD; // card exists to make bit 26 as '0'.

	//Jason, Apr/19/2011.
	REG_FCR_FUNC_CTRL |= FCR_FUNC_CTRL_LITTLE_ENDIAN;  // set to little endian

	printk ("DRV_SDC_SysInit : REG_FCR_FUNC_CTRL = 0x%lx\n", REG_FCR_FUNC_CTRL);
	//Ben, Apr/18/2011.
	REG_FCR_CPBLT &= (~(1<<20)); // select eMMC bus but not NAND bus.
	printk ("DRV_SDC_SysInit : REG_FCR_CPBLT = 0x%lx\n", REG_FCR_CPBLT);

	return TRUE;
}

static void DRV_SDC_Init(void)
{
	printk("=>DRV_SDC_InitCard!\n");

	
	#ifdef NT72558_eMMC_DEBUG
	*(volatile unsigned long*)(0xBC040204) = 0x72682;
	*(volatile unsigned long*)(0xBC040204) = 0x28627;
	*(volatile unsigned long*)(0xBC040208) = 1;
	*(volatile unsigned long*)(0xBD0F0008) |= (1<<4); //set GPA4 as output pin.
	#endif

	
	REG_NFC_SYS_CTRL |= Emmc_Sel;
	DRV_SDC_SysInit();

	REG_SDC_SW_RESET = SDC_SW_RESET_ALL | SDC_SW_RESET_CMD_LINE | SDC_SW_RESET_DAT_LINE;
	while(REG_SDC_SW_RESET & (SDC_SW_RESET_ALL | SDC_SW_RESET_CMD_LINE | SDC_SW_RESET_DAT_LINE))
		;

	REG_FCR_CPBLT &= ~(1<<19);
	REG_SDC_TIMEOUT_CTRL = SDC_TIMEOUT_CTRL_DATA_TIMEOUT(0xa);
	
	REG_FCR_FUNC_CTRL = (REG_FCR_FUNC_CTRL & ~FCR_FUNC_CTRL_SD_SIG_DELAY_MASK) |
	FCR_FUNC_CTRL_SD_SIG_DELAY(3);
	*(volatile unsigned int *)(0xbc048200) |= 0x30000000;
	REG_SDC_PW_CTRL = SDC_PW_CTRL_BUS_PW_ON | SDC_PW_CTRL_BUS_VOL_33V;
	mdelay(35);
	DRV_SDC_InitClock(SDC_CLK2DIV(400000));
}

static void emmc_set_clock(struct sdhci_host *host, unsigned int clock)
{
	unsigned int clock_div;

	if(clock == 0 || clock == host->clock)
		goto end;

	clock_div = SDC_CLK2DIV(clock);
	DRV_SDC_InitClock(clock_div);

end:
	host->clock = clock;
	return;
}

static unsigned int emmc_get_max_clock(struct sdhci_host *host)
{

	unsigned int Max_Clock_Setting;

	Max_Clock_Setting = 52000000;

	if (Max_Clock_Setting > 40000000)
	{
		//Set eMMC DATA0~7 as higher driving current.	
		
		/*
		PAD_ONENANDDQ0	  : 0xBC04_0838[9:8]		
		PAD_ONENANDDQ1	  : 0xBC04_0838[17:16]		
		PAD_ONENANDDQ2	  : 0xBC04_0838[25:24]		
		PAD_ONENANDDQ3	  : 0xBC04_083C[1:0]		
		PAD_ONENANDDQ4	  : 0xBC04_083C[9:8]		
		PAD_ONENANDDQ5	  : 0xBC04_083C[17:16]		
		PAD_ONENANDDQ6	  : 0xBC04_083C[25:24]		
		PAD_ONENANDDQ7	  : 0xBC04_0840[1:0]		 
		
		Setting : 		
		00 : 2.5 mA 					  01 : 5 mA		
		10 : 7.5 mA 					  11 : 10 mA
		*/

		*(volatile unsigned long*)(0xBC040204) = 0x72682;
		*(volatile unsigned long*)(0xBC040204) = 0x28627;
		*(volatile unsigned long*)(0xBC040208) = 1;		
		*(volatile unsigned long*)(0xBC040838) = ( *(volatile unsigned long*)(0xBC040838) & 0xfcfcfcff ) | (2<<8) | (2<<16) | (2<<24);

		*(volatile unsigned long*)(0xBC040204) = 0x72682;
		*(volatile unsigned long*)(0xBC040204) = 0x28627;
		*(volatile unsigned long*)(0xBC040208) = 1;
		*(volatile unsigned long*)(0xBC04083c) = ( *(volatile unsigned long*)(0xBC04083c) & 0xfcfcfcfc ) | (2<<0) | (2<<8) | (2<<16) | (2<<24);

		*(volatile unsigned long*)(0xBC040204) = 0x72682;
		*(volatile unsigned long*)(0xBC040204) = 0x28627;
		*(volatile unsigned long*)(0xBC040208) = 1;
		*(volatile unsigned long*)(0xBC040840) = ( *(volatile unsigned long*)(0xBC040840) & 0xfffffffc ) | (2<<0);
		
	}

	return Max_Clock_Setting;
	
}

static unsigned int emmc_get_min_clock(struct sdhci_host *host)
{
	return 400000;
}

static int nt72558_emmc_init(struct sdhci_host *host)
{
	volatile u32 iectype_low;

	//change irq to level trigger 
	iectype_low = *(volatile u32 *)(0xbd0e0018);
        *(volatile u32 *)(0xbd0e0018) = iectype_low & (~(1<<20));

	host->ioaddr = (void *)0xbc048300;
	DRV_SDC_Init();

	return 0;
}

static void nt72558_emmc_exit(struct sdhci_host *host)
{
	return;
}

static struct sdhci_ops nt72558_emmc_ops = {
	.set_clock = emmc_set_clock,
	.get_max_clock = emmc_get_max_clock,
	.get_min_clock = emmc_get_min_clock,
};

static struct resource NT72558_eMMC[] = {
	[0] = {
		.start          = 0xbc048200,
		.end            = 0xbc0483ff,
		.flags          = IORESOURCE_MEM,
	},
	[1] = {
		.start          = 20,
		.end            = 20,
		.flags          = IORESOURCE_IRQ,
	},
};

static struct sdhci_pltfm_data nt72558_sdhci_data = {
	.ops = &nt72558_emmc_ops,
	.quirks = SDHCI_QUIRK_NONSTANDARD_CLOCK | SDHCI_QUIRK_SINGLE_POWER_WRITE | SDHCI_QUIRK_CAP_CLOCK_BASE_BROKEN,
	.init = nt72558_emmc_init,
	.exit = nt72558_emmc_exit,
};

static struct platform_device nt72558_emmc_device = {
	.name           = "sdhci",
	.id             = 0,
	.num_resources  = ARRAY_SIZE(NT72558_eMMC),
	.resource       =  NT72558_eMMC,
	.dev            = {
		.platform_data = &nt72558_sdhci_data,
	}
};

static int __init nt72558_add_emmc(void)
{
	return platform_device_register(&nt72558_emmc_device);
}

device_initcall(nt72558_add_emmc);

