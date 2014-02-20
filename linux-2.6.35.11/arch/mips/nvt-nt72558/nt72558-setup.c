/*
 *  This program is free software; you can distribute it and/or modify it
 *  under the terms of the GNU General Public License (Version 2) as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 */


#include <linux/cpu.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/ioport.h>
#include <linux/irq.h>
#include <linux/time.h>

#include <asm/bootinfo.h>
#include <asm/mips-boards/generic.h>
#include <asm/mips-boards/prom.h>
#include <asm/mips-boards/nvt72558_int.h>
#include <asm/mips-boards/nvt72558.h>
#include <linux/serial.h>
#include <linux/serial_core.h>
#include <linux/serial_8250.h>
#include <asm/dma.h>
#include <asm/traps.h>

extern void malta_be_init(void);
extern int malta_be_handler(struct pt_regs *regs, int is_fixup);
extern void mips_reboot_setup(void);

extern unsigned long get_sys_mpll(unsigned long);

const char *get_system_type(void)
{
	return "NVT NT72558";
}

const char display_string[] = "        LINUX ON NVT NT72558       ";
static void __init clk_init(void);
unsigned long hclk = 0;

extern unsigned int PCI_DMA_BUS_IS_PHYS;

void __init plat_mem_setup(void)
{
	mips_reboot_setup();
	clk_init ();

	board_be_init = malta_be_init;
	board_be_handler = malta_be_handler;

	/*
	 * make USB work under HIGHMEM
	 */ 
	PCI_DMA_BUS_IS_PHYS = 1;

}

static void __init clk_init(void)
{
#ifndef CONFIG_AHB_CLOCK
  unsigned int ratio;
  unsigned int temp;

  ratio = AHB_CLK_SEL;
  /* source is CPUPLL */
  if ((ratio == 0) || (ratio == 1)) 
	{      
		temp = get_sys_mpll(_CPU_PLL_OFFSET);
    if (ratio == 0)
		{
      hclk = temp/6;
    }
    else //if (ratio == 1) 
		{
      hclk = temp/4;
    }
  }
  else if (ratio == 2){
    /* source is BODAPLL */
	}
  else if (ratio == 3){
    /* source is AXIPLL */
		temp = get_sys_mpll(_AXI_PLL_OFFSET);

    hclk = temp/4;
  }
	else // if (ratio >= 4)
	{
		/* source is reference clock 12M*16 */
		hclk = 12*16/2;
	}
	hclk *= 1000;
#else
  hclk = CONFIG_AHB_CLOCK*1000;
#endif
	printk("hclk = %ld \n", hclk);
}

EXPORT_SYMBOL(hclk);

