#ifndef __ASM_MACH_MIPS_IRQ_H
#define __ASM_MACH_MIPS_IRQ_H

#if CONFIG_MSTAR_JANUS2
#define GIC_NUM_INTRS 168
#define NR_IRQS 302
#else
#define NR_IRQS	256
#endif

#include_next <irq.h>

#endif /* __ASM_MACH_MIPS_IRQ_H */
