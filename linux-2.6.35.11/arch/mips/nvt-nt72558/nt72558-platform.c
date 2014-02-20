/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 */




#include <linux/init.h>
#include <linux/serial_8250.h>
#include <linux/mc146818rtc.h>
#include <linux/module.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/physmap.h>
#include <linux/platform_device.h>
#include <mtd/mtd-abi.h>

#include <asm/mips-boards/nvt72558.h>
#include <asm/mips-boards/nvt72558_int.h>

/* Declare in nt72558-setup.c */
extern unsigned long hclk;

#define SMC_PORT(base, int)						\
{									\
	.membase	= (unsigned char* __iomem)base,			\
	.irq		= int,						\
	.iotype		= UPIO_MEM32,					\
	.flags		= UPF_BOOT_AUTOCONF | UPF_SKIP_TEST,		\
	.regshift	= 2,						\
}

static struct resource NT72558_USB0[] = {
	[0] = {
		.start          = 0xbc140000,
		.end            = 0xbc140fff,
		.flags          = IORESOURCE_MEM,
	},
	[1] = {
		.start          = 5,
		.end            = 5,
		.flags          = IORESOURCE_IRQ,
	},
};

static struct resource NT72558_USB1[] = {
	[0] = {
		.start          = 0xbc148000,
		.end            = 0xbc148fff,
		.flags          = IORESOURCE_MEM,
	},
	[1] = {
		.start          = 23,
		.end            = 23,
		.flags          = IORESOURCE_IRQ,
	},
};
static struct plat_serial8250_port uart8250_data[] = {
	{
		.membase	= (unsigned char* __iomem)NT72558_UART1_REGS_BASE,
		.mapbase    = (unsigned int)NT72558_UART1_REGS_BASE,
		.irq		= NT72558_INT_UART1,
		.iotype		= UPIO_MEM32,
		.flags		= UPF_BOOT_AUTOCONF | UPF_SKIP_TEST,
		.regshift	= 2,
	},
	{
		.iotype		= UPIO_MEM32,
		.flags		= UPF_BOOT_AUTOCONF | UPF_SKIP_TEST,
		.regshift	= 2,
	},
	//	SMC_PORT(NT72558_UART1_REGS_BASE, NT72688_INT_UART1),
//	SMC_PORT(NT72558_UART0_REGS_BASE, NT72688_INT_UART0),
	{ },
};

static struct platform_device nt72558_uart8250_device = {
	.name			= "serial8250",
	.id			= PLAT8250_DEV_PLATFORM,
	.dev			= {
		.platform_data	= uart8250_data,
	},
};

#ifdef CONFIG_HIGHMEM
static u64 ehci_dmamask = HIGHMEM_START;
#else
static u64 ehci_dmamask = ~(u32)0;
#endif

static struct platform_device NT72558_USB0_device = {
	.name           = "NT72558-ehci",
	.id             = 0,
	.dev = {
		.dma_mask               = &ehci_dmamask,
		.coherent_dma_mask      = 0xffffffff,
	},
	.num_resources  = ARRAY_SIZE(NT72558_USB0),
	.resource       =  NT72558_USB0,
};


static struct platform_device NT72558_USB1_device = {
	.name           = "NT725581-ehci",
	.id             = 0,
	.dev = {
		.dma_mask               = &ehci_dmamask,
		.coherent_dma_mask      = 0xffffffff,
	},
	.num_resources  = ARRAY_SIZE(NT72558_USB1),
	.resource       =  NT72558_USB1,
};


static struct platform_device *nt72558_devices[] __initdata = {
	&nt72558_uart8250_device,
	&NT72558_USB0_device,
	&NT72558_USB1_device,
};

static int __init nt72558_add_devices(void)
{
	int err;
	int set;

	if (!hclk)
		printk ("Error hclk setting ....\n");

	//CHECK the IO setting
	set = GPIO_LV_VALUE(4);

	if( 0 == set)		//GPIO4 low
	{
		uart8250_data[1].membase	= (unsigned char* __iomem)NT72558_UART0_REGS_BASE;
		uart8250_data[1].mapbase	= (unsigned int)NT72558_UART0_REGS_BASE;
		uart8250_data[1].irq		= NT72558_INT_UART0;
	}
	else
	{
		uart8250_data[1].membase	= (unsigned char* __iomem)NT72558_UART2_REGS_BASE;
		uart8250_data[1].mapbase	= (unsigned int)NT72558_UART2_REGS_BASE;
		uart8250_data[1].irq		= NT72558_INT_UART2;
	}

	uart8250_data[0].uartclk = hclk;
	uart8250_data[1].uartclk = hclk;

#if 0
	REG_SYSTEM_CONTROL   |= SYSTEM_UART0_MASK;
	REG_SYSTEM_CONTROL_1 |= SYSTEM_UART1_MASK;
#endif

	err = platform_add_devices(nt72558_devices, ARRAY_SIZE(nt72558_devices));
	if (err)
		return err;

	return 0;
}

device_initcall(nt72558_add_devices);
