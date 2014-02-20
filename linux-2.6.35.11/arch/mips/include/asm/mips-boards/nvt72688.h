/*
 * 
 * ########################################################################
 *
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
 * ########################################################################
 *
 * Defines of REGs and Address of NVT NT72688 SOC
 *
 */
#ifndef _MIPS_NVT72688_H
#define _MIPS_NVT72688_H

#include <asm/addrspace.h>

#define SYSTEM_REG_BASE		0xBC0D0000

/* system register */
#define REG_SYSTEM_CONTROL	*((volatile unsigned int *) (SYSTEM_REG_BASE + 0x00))
#define REG_SYSTEM_CONTROL_1	*((volatile unsigned int *) (SYSTEM_REG_BASE + 0x04))

#define SYSTEM_CLK_REG_BASE	0xBC0C0000

/* cpu pll register */
#define REG_CLOCK_SELECT	*((volatile unsigned int *) (SYSTEM_CLK_REG_BASE + 0x0014))
#define REG_CPUPLL_RATIO	*((volatile unsigned int *) (SYSTEM_CLK_REG_BASE + 0x1160))
#define REG_CPUPLL_RATIO_1	*((volatile unsigned int *) (SYSTEM_CLK_REG_BASE + 0x1164))
#define REG_AXIPLL_RATIO	*((volatile unsigned int *) (SYSTEM_CLK_REG_BASE + 0x1168))
#define REG_AXIPLL_RATIO_1	*((volatile unsigned int *) (SYSTEM_CLK_REG_BASE + 0x116C))

#define SYSTEM_UART0_MASK	0x00010000
#define SYSTEM_UART1_MASK	0x00000010

/*
 * UART register base.
 */
#define NT72688_UART0_REGS_BASE    (0xBD090000)
#define NT72688_UART1_REGS_BASE    (0xBD091000)
#define NT72688_BASE_BAUD ( 115200 / 16 )


/*
 * INTERRUPT CONTROLLER
 */

#define REG_CPU0_IRQ_ID      *((volatile unsigned int *) (0xBD0E0080))
#define REG_CPU0_IRQ_MASK_L  *((volatile unsigned int *) (0xBD0E0010))
#define REG_CPU0_IRQ_MASK_H  *((volatile unsigned int *) (0xBD0E0014))

#define ADDR_CPU0_IRQ_MASK_L (0xBD0E0010)
#define ADDR_CPU0_IRQ_MASK_H (0xBD0E0014)

#endif /* !(_MIPS_NVT72688_H) */
