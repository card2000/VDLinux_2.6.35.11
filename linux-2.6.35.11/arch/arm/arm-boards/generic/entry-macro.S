/*
 * include/asm-arm/arch-versatile/entry-macro.S
 *
 * Low-level IRQ helper macros for Versatile platforms
 *
 * This file is licensed under  the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include <mach/hardware.h>
#include <asm/hardware/vic.h>

		.macro	disable_fiq
		.endm

		.macro  get_irqnr_preamble, base, tmp
		/*ldr	\base, =IO_ADDRESS(VERSATILE_VIC_BASE) */
		.endm

		.macro  arch_ret_to_user, tmp1, tmp2
		.endm

		/*
		This macro is used to get trigger irq number: irqnr
		and the irqstat is the status register of the interrupt controller.
		the irq will be raised if irq or fiq occurred
		*/
		.macro	get_irqnr_and_base, irqnr, irqstat, base, tmp

#if defined(CONFIG_ARCH_PIONEER)
		ldr 	\tmp, =0xe0000a8c /* irq index register */
#elif defined(CONFIG_ARCH_COLUMBUS)
    ldr 	\tmp, =0xe000568c /* irq index register */
#else
#error "Unknown Machine Type "
#endif

		ldr	\irqnr, [\tmp]
		/*b .	*/
		cmp 	\irqnr, #0xff /* test if there is any irq */
		beq 	101f /* branch if there isn't any irq */

		/* here we should get the interrupt status into the \irqstat,
		but it seems that this information is not very useful outside the macro

#if defined(CONFIG_ARCH_PIONEER)
		ldr 	\tmp, =0xe0000a24
#elif defined(CONFIG_ARCH_COLUMBUS)
    ldr 	\tmp, =0xe0005624
#else
#error "Unknown Machine Type "
#endif

		ldr 	\irqstat, [tmp]
		mov 	\irqstat, \irqstat
		*/

101:	   	/* EQ will be set if \irqnr = 0xff */

		.endm

