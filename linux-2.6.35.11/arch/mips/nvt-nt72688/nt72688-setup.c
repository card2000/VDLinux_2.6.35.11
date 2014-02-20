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
#include <asm/mips-boards/nvt72688_int.h>
#include <asm/mips-boards/nvt72688.h>
#include <linux/serial.h>
#include <linux/serial_core.h>
#include <linux/serial_8250.h>
#include <asm/dma.h>
#include <asm/traps.h>

extern void malta_be_init(void);
extern int malta_be_handler(struct pt_regs *regs, int is_fixup);
extern void mips_reboot_setup(void);



const char *get_system_type(void)
{
	return "NVT NT72688";
}

const char display_string[] = "        LINUX ON NVT NT72688       ";
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

  ratio  = ((REG_CLOCK_SELECT >> 24) & 3);
  /* source is CPUPLL */
  if (ratio < 3) {      
    ratio = (REG_CPUPLL_RATIO >> 16);
    ratio |= (REG_CPUPLL_RATIO_1 << 16);
    ratio >>= (12 + (REG_CPUPLL_RATIO & 7)); /* pre-scale down becaus of size limit */
    hclk  = ((12000 * ratio) >> 5) * 1000;
    ratio = ((REG_CLOCK_SELECT >> 24) & 3);
    if (ratio == 0) {
      hclk /= 6;
    }
    else if (ratio == 1) {
      hclk /= 4;
    }
    else {
      /* if (ratio == 2) */
      hclk /= 3;
    }
  }
  else {
    /* source is AXIPLL */
    ratio = (REG_AXIPLL_RATIO >> 16);
    ratio |= (REG_AXIPLL_RATIO_1 << 16);
    ratio >>= (12 + (REG_AXIPLL_RATIO & 7)); /* pre-scale down becaus of size limit */
    hclk  = ((12000 * ratio) >> 5) * 1000;

    hclk /= 4;
  }
#else
  hclk = CONFIG_AHB_CLOCK*1000;
#endif
	printk("hclk = %ld \n", hclk);
}
