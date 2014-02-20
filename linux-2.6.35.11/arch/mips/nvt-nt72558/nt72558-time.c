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
 *
 */

#include <linux/types.h>
#include <linux/init.h>
#include <linux/kernel_stat.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/timex.h>
#include <linux/mc146818rtc.h>

#include <asm/mipsregs.h>
#include <asm/mipsmtregs.h>
#include <asm/hardirq.h>
#include <asm/irq.h>
#include <asm/div64.h>
#include <asm/cpu.h>
#include <asm/time.h>
#include <asm/mc146818-time.h>
#include <asm/msc01_ic.h>

#include <asm/mips-boards/generic.h>
#include <asm/mips-boards/prom.h>
#include <asm/mips-boards/nvt72558.h>
#include <asm/mips-boards/nvt72558_int.h>
#include <asm/mips-boards/maltaint.h>

unsigned long cpu_khz;

static int mips_cpu_timer_irq;
extern int cp0_perfcount_irq;

extern unsigned long get_sys_mpll(unsigned long);

static void mips_timer_dispatch(void)
{
	do_IRQ(mips_cpu_timer_irq);
}

/*
 * Estimate CPU frequency.  Sets mips_hpt_frequency as a side-effect
 */
static unsigned int __init estimate_cpu_frequency(void)
{
  unsigned int prid = read_c0_prid() & 0xffff00;
  unsigned int count;

#ifndef CONFIG_CPU_CLOCK
  unsigned int ratio;

	count = get_sys_mpll(_CPU_PLL_OFFSET);
	ratio = 4 - MIPS_CLK_SEL;
	count /= ratio;
	count *= 500;
#else
	count = CONFIG_CPU_CLOCK*500;           /*unit of CONFIG_CPU_CLOCK is KHz*/
#endif

  mips_hpt_frequency = count;
  if ((prid != (PRID_COMP_MIPS | PRID_IMP_20KC)) &&
      (prid != (PRID_COMP_MIPS | PRID_IMP_25KF)))
    count *= 2;

  count += 5000;    /* round */
  count -= count%10000;

  return count;
}


#if 0
unsigned long read_persistent_clock(void)
{
	return mc146818_get_cmos_time();
}
#endif


unsigned int __cpuinit get_c0_compare_int(void)
{
	if (cpu_has_vint)
		set_vi_handler(mips_cpu_timer_irq, mips_timer_dispatch);
	mips_cpu_timer_irq = NT72558_INT_BASE + NT72558_INT_CPUCTR;

	return mips_cpu_timer_irq;
}

void __init plat_time_init(void)
{
	unsigned int est_freq;

	est_freq = estimate_cpu_frequency();

	printk("CPU frequency %d.%02d MHz\n", est_freq/1000000,
	       (est_freq%1000000)*100/1000000);

	cpu_khz = est_freq / 1000;

	mips_scroll_message();

}
