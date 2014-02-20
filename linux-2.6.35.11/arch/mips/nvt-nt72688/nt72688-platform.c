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

#include <asm/mips-boards/nvt72688.h>
#include <asm/mips-boards/nvt72688_int.h>

/* Declare in nt72688-setup.c */
extern unsigned long hclk;

#define SMC_PORT(base, int)						\
{									\
	.membase	= (unsigned char* __iomem)base,			\
	.irq		= int,						\
	.iotype		= UPIO_MEM32,					\
	.flags		= UPF_BOOT_AUTOCONF | UPF_SKIP_TEST,		\
	.regshift	= 2,						\
}

static struct resource NT72688_USB0[] = {
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

static struct resource NT72688_USB1[] = {
	[0] = {
		.start          = 0xbc141000,
		.end            = 0xbc141fff,
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
		.membase	= (unsigned char* __iomem)NT72688_UART0_REGS_BASE,
		.mapbase    = (unsigned int)NT72688_UART0_REGS_BASE,
		.irq		= NT72688_INT_UART0,
		.iotype		= UPIO_MEM32,
		.flags		= UPF_BOOT_AUTOCONF | UPF_SKIP_TEST,
		.regshift	= 2,
	},
	{
		.membase	= (unsigned char* __iomem)NT72688_UART1_REGS_BASE,
		.mapbase    = (unsigned int)NT72688_UART1_REGS_BASE,
		.irq		= NT72688_INT_UART1,
		.iotype		= UPIO_MEM32,
		.flags		= UPF_BOOT_AUTOCONF | UPF_SKIP_TEST,
		.regshift	= 2,
	},
//	SMC_PORT(NT72688_UART1_REGS_BASE, NT72688_INT_UART1),
//	SMC_PORT(NT72688_UART0_REGS_BASE, NT72688_INT_UART0),
	{ },
};

static struct platform_device nt72688_uart8250_device = {
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

static struct platform_device NT72688_USB0_device = {
	.name           = "NT72682-ehci",
	.id             = 0,
	.dev = {
		.dma_mask               = &ehci_dmamask,
		.coherent_dma_mask      = 0xffffffff,
	},
	.num_resources  = ARRAY_SIZE(NT72688_USB0),
	.resource       =  NT72688_USB0,
};


static struct platform_device NT72688_USB1_device = {
	.name           = "NT72683-ehci",
	.id             = 0,
	.dev = {
		.dma_mask               = &ehci_dmamask,
		.coherent_dma_mask      = 0xffffffff,
	},
	.num_resources  = ARRAY_SIZE(NT72688_USB1),
	.resource       =  NT72688_USB1,
};


static struct platform_device *nt72688_devices[] __initdata = {
	&nt72688_uart8250_device,
	&NT72688_USB0_device,
	&NT72688_USB1_device,
};

static int __init nt72688_add_devices(void)
{
	int err;

	if (!hclk)
		printk ("Error hclk setting ....\n");

	uart8250_data[0].uartclk = hclk;
	uart8250_data[1].uartclk = hclk;

	REG_SYSTEM_CONTROL   |= SYSTEM_UART0_MASK;
        REG_SYSTEM_CONTROL_1 |= SYSTEM_UART1_MASK;

	err = platform_add_devices(nt72688_devices, ARRAY_SIZE(nt72688_devices));
	if (err)
		return err;

	return 0;
}

device_initcall(nt72688_add_devices);
