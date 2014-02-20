/*
 * arch/arm/mach-ccep/include/mach/entry-macro.S
 *
 * Samsung DTV Soc interrupt Assembler
 * Copyright (C) 2010 tukho.kim@samsung.com
 *
 */

#include <mach/hardware.h>
#include <asm/hardware/gic.h>
#include <plat/sdp_irq.h>

.macro  disable_fiq
.endm

.macro arch_ret_to_user, tmp1, tmp2
.endm

.macro get_irqnr_preamble, base, tmp
#ifdef CONFIG_ARM_GIC
	movw	\base, #(VA_GIC_CPU_BASE & 0xffff)
	movt	\base, #((VA_GIC_CPU_BASE & 0xffff0000) >> 16)
#endif
.endm

/* Try to semi vectored interrupt.
 * if using semi vectored interrupt, must set semi-vectored
 * interrupt in INTCON register
 * tukho.kim@samsung.com 2010/06  */

.macro get_irqnr_and_base, irqnr, irqstat, base, tmp

#ifdef CONFIG_ARM_GIC

	ldr	\irqstat, [\base, #GIC_CPU_INTACK] /* bits 12-10 = src CPU, 9-0 = int # */

	ldr	\tmp, =1021

	bic	\irqnr, \irqstat, #0x1c00

	cmp	\irqnr, #29
	cmpcc	\irqnr, \irqnr
	cmpne	\irqnr, \tmp
	cmpcs	\irqnr, \irqnr

	.endm

	/* We assume that irqstat (the raw value of the IRQ acknowledge
	* register) is preserved from the macro above.
	* If there is an IPI, we immediately signal end of interrupt on the
	* controller, since this requires the original irqstat value which
	* we won't easily be able to recreate later.
	*/

	.macro test_for_ipi, irqnr, irqstat, base, tmp
	bic	\irqnr, \irqstat, #0x1c00
	cmp	\irqnr, #16
	strcc	\irqstat, [\base, #GIC_CPU_EOI]
	cmpcs	\irqnr, \irqnr
	.endm

	/* As above, this assumes that irqstat and base are preserved.. */

	.macro test_for_ltirq, irqnr, irqstat, base, tmp
	bic	\irqnr, \irqstat, #0x1c00
	mov	\tmp, #0
	cmp	\irqnr, #29
	moveq	\tmp, #1
	streq	\irqstat, [\base, #GIC_CPU_EOI]
	cmp	\tmp, #0

#else

	mov     \base, #SDP_INTC_BASE0&0xff000000 @ IO Base add virtual
	add     \base, \base, #SDP_INTC_BASE0&0x00ff0000
	add     \base, \base, #SDP_INTC_BASE0&0x0000ff00

	@ read rI_ISPR register check, 
	@ if Pend regiser is 0, just out
	ldr	\irqstat, [\base, #I_ISPR]      @ get interrupts bits
	cmp	\irqstat, #0

#if defined(CONFIG_SDP_INTC64)
	beq	1001f				@ 2nd interrupt controller
#else
	moveq	\irqnr, #IRQ_TIMER
#endif
	ldrne	\irqstat, [\base, #0x78]
	movne	\irqnr, \irqstat, LSR #0x2

#if defined(CONFIG_SDP_INTC64)
	bne	1002f
1001:
#if (SDP_INTC_BASE1 > SDP_INTC_BASE0)
	add     \base, \base, #(SDP_INTC_BASE1 - SDP_INTC_BASE0)
#else
	add     \base, \base, #(SDP_INTC_BASE0 - SDP_INTC_BASE1)
#endif
	ldr	\irqstat, [\base, #I_ISPR]      @ get interrupts bits
	cmp	\irqstat, #0
	
	moveq	\irqnr, #IRQ_TIMER

	ldrne	\irqstat, [\base, #0x78]
	movne	\irqnr, \irqstat, LSR #0x2
	addne 	\irqnr, \irqnr, #32
1002:
#endif
#endif
.endm


