#ifndef __ASM_MACH_MIPS_IRQ_H
#define __ASM_MACH_MIPS_IRQ_H

#ifdef CONFIG_MSTAR_CHIP
#define GIC_NUM_INTRS	(24 + NR_CPUS * 2)
#endif
#define NR_IRQS	256

#include_next <irq.h>

#endif /* __ASM_MACH_MIPS_IRQ_H */
