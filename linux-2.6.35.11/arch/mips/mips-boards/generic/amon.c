/*
 * Copyright (C) 2007  MIPS Technologies, Inc.
 *	All rights reserved.

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
 * Arbitrary Monitor interface
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/smp.h>

#include <asm/addrspace.h>
#include <asm/mips-boards/launch.h>
#include <asm/mipsmtregs.h>

int amon_cpu_avail(int cpu)
{
	//struct cpulaunch *launch = (struct cpulaunch *)CKSEG0ADDR(CPULAUNCH);
	struct cpulaunch *launch = (struct cpulaunch *) &__cpulaunch;

	if (cpu < 0 || cpu >= NCPULAUNCH) {
		pr_debug("avail: cpu%d is out of range\n", cpu);
		return 0;
	}

	launch += cpu;
	if (!(launch->flags & LAUNCH_FREADY)) {
		pr_debug("avail: cpu%d is not ready\n", cpu);
		return 0;
	}
	if (launch->flags & (LAUNCH_FGO|LAUNCH_FGONE)) {
		pr_debug("avail: too late.. cpu%d is already gone\n", cpu);
		return 0;
	}

	return 1;
}

void amon_cpu_start(int cpu,
		    unsigned long pc, unsigned long sp,
		    unsigned long gp, unsigned long a0)
{
//	volatile struct cpulaunch *launch = (struct cpulaunch  *)CKSEG0ADDR(CPULAUNCH);
	volatile struct cpulaunch *launch = (struct cpulaunch  *)&__cpulaunch;

    if((unsigned long)&__cpulaunch != 0x80000F00)
    {
        printk(KERN_ERR"%s %d : ", __FUNCTION__,__LINE__);
        printk(KERN_ERR"Currently sboot write the cpu launch flag on 0x80000f00\n please sync the address with sboot\n");
    }

	if (!amon_cpu_avail(cpu))
		return;
	if (cpu == smp_processor_id()) {
		pr_debug("launch: I am cpu%d!\n", cpu);
		return;
	}
	launch += cpu;

	pr_debug("launch: starting cpu%d\n", cpu);

	launch->pc = pc;
	launch->gp = gp;
	launch->sp = sp;
	launch->a0 = a0;

	/* Make sure target sees parameters before the go bit */
	smp_mb();

	launch->flags |= LAUNCH_FGO;
	while ((launch->flags & LAUNCH_FGONE) == 0)
		;
	pr_debug("launch: cpu%d gone!\n", cpu);
}

/*
 * Restart the CPU ready for amon_cpu_start again
 * A bit of a hack... it duplicates the CPU startup code that
 * lives somewhere in the boot monitor.
 */
void amon_cpu_dead(void)
{
	struct cpulaunch *launch;
	unsigned int r0;
	int cpufreq;

	//launch = (struct cpulaunch *)CKSEG0ADDR(CPULAUNCH);
	launch = (struct cpulaunch *) &__cpulaunch;
	launch += smp_processor_id();

	launch->pc = 0;
	launch->gp = 0;
	launch->sp = 0;
	launch->a0 = 0;
	launch->flags = LAUNCH_FREADY;

	cpufreq = 600000000;	/* FIXME: Correct value? */

	asm volatile (
"	.set push\n"
"	.set noreorder\n"
"1:\n"
"	lw	%[r0],%[LAUNCH_FLAGS](%[launch])\n"
"	andi	%[r0],%[FGO]\n"
"	bnez    %[r0],1f\n"
"        nop\n"
"	mfc0	%[r0],$9\n"	/* Read Count */
"	addu    %[r0],%[waitperiod]\n"
"	mtc0	%[r0],$11\n"	/* Write Compare */
"	wait\n"
"	b	1b\n"
"        nop\n"
"1:	lw	$ra,%[LAUNCH_PC](%[launch])\n"
"	lw	$gp,%[LAUNCH_GP](%[launch])\n"
"	lw	$sp,%[LAUNCH_SP](%[launch])\n"
"	lw	$a0,%[LAUNCH_A0](%[launch])\n"
"	move	$a1,$zero\n"
"	move	$a2,$zero\n"
"	move	$a3,$zero\n"
"	lw	%[r0],%[LAUNCH_FLAGS](%[launch])\n"
"	ori	%[r0],%[FGONE]\n"
"	jr	$ra\n"
"	 sw	%[r0],%[LAUNCH_FLAGS](%[launch])\n"
"	.set	pop\n"
	: [r0] "=&r" (r0)
	: [launch] "r" (launch),
	  [FGONE] "i" (LAUNCH_FGONE),
	  [FGO] "i" (LAUNCH_FGO),
	  [LAUNCH_PC] "i" (offsetof(struct cpulaunch, pc)),
	  [LAUNCH_GP] "i" (offsetof(struct cpulaunch, gp)),
	  [LAUNCH_SP] "i" (offsetof(struct cpulaunch, sp)),
	  [LAUNCH_A0] "i" (offsetof(struct cpulaunch, a0)),
	  [LAUNCH_FLAGS] "i" (offsetof(struct cpulaunch, flags)),
	  [waitperiod] "i" ((cpufreq / 2) / 100)	/* delay of ~10ms  */
	: "ra","a0","a1","a2","a3","sp" /* ,"gp" */
	);
}
