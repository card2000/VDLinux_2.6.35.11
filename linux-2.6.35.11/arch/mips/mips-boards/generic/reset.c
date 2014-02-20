/*
 * Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 1999,2000 MIPS Technologies, Inc.  All rights reserved.
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
 * Reset the MIPS boards.
 *
 */
#include <linux/pm.h>

#include <asm/io.h>
#include <asm/reboot.h>
#include <asm/mips-boards/generic.h>
#if defined(CONFIG_MIPS_ATLAS)
#include <asm/mips-boards/atlas.h>
#endif

#if defined(CONFIG_MSTAR_TITANIA3) | defined(CONFIG_MSTAR_TITANIA4) | defined(CONFIG_MSTAR_URANUS4) | defined(CONFIG_MSTAR_TITANIA8) | defined(CONFIG_MSTAR_TITANIA9) | defined(CONFIG_MSTAR_JANUS2) | defined(CONFIG_MSTAR_TITANIA11) | defined(CONFIG_MSTAR_TITANIA12) | defined(CONFIG_MSTAR_TITANIA13) | defined(CONFIG_MSTAR_AMBER1) | defined(CONFIG_MSTAR_AMBER2) | defined(CONFIG_MSTAR_AMBER5) | defined(CONFIG_MSTAR_KRONUS)
#define REG_WDT_BASE    0xbf006000
#elif defined(CONFIG_MSTAR_EUCLID)
#define REG_WDT_BASE    0xbf0078c0
#else // CONFIG_MSTAR_TITANIA2
#define REG_WDT_BASE    0xbf8078c0
#endif

static void mips_machine_restart(char *command);
static void mips_machine_halt(void);
#if defined(CONFIG_MIPS_ATLAS)
static void atlas_machine_power_off(void);
#endif


#if defined(CONFIG_MSTAR_TITANIA3) ||   \
    defined(CONFIG_MSTAR_TITANIA4) ||   \
    defined(CONFIG_MSTAR_URANUS4) ||    \
    defined(CONFIG_MSTAR_TITANIA13) ||  \
    defined(CONFIG_MSTAR_AMBER1) ||     \
    defined(CONFIG_MSTAR_KRONUS)
#define REG_PM_SLEEP_BASE    0xbf001C00
#elif defined(CONFIG_MSTAR_TITANIA8) || \
      defined(CONFIG_MSTAR_TITANIA12) ||    \
      defined(CONFIG_MSTAR_AMBER2)||      \
      defined(CONFIG_MSTAR_AMBER5)||      \
      defined(CONFIG_MSTAR_JANUS2)
#define REG_PM_MSIC_BASE	 0xbf005C00
#define REG_PM_SLEEP_BASE    REG_PM_MSIC_BASE
#endif
static void mips_machine_restart(char *command)
{
printk("mips_machine_restart\n");

#if defined(CONFIG_MSTAR_TITANIA3) ||   \
    defined(CONFIG_MSTAR_TITANIA4) ||   \
    defined(CONFIG_MSTAR_URANUS4) ||    \
    defined(CONFIG_MSTAR_TITANIA8) ||   \
    defined(CONFIG_MSTAR_TITANIA9) ||   \
    defined(CONFIG_MSTAR_JANUS2) ||     \
    defined(CONFIG_MSTAR_TITANIA11) ||  \
    defined(CONFIG_MSTAR_TITANIA12) ||  \
    defined(CONFIG_MSTAR_TITANIA13) ||  \
    defined(CONFIG_MSTAR_AMBER1) ||     \
    defined(CONFIG_MSTAR_AMBER2)||      \
    defined(CONFIG_MSTAR_AMBER5)||      \
    defined(CONFIG_MSTAR_KRONUS)

    // fire reset for global chip, active by password (8'h79)
    // 1. set to default first
    *(volatile unsigned long*)(REG_PM_SLEEP_BASE + 0xB8) = 0xff;    // reg_top_sw_rst
    // 2. active by password (8'h79)
    *(volatile unsigned long*)(REG_PM_SLEEP_BASE + 0xB8) = 0x79;    // reg_top_sw_rst
#endif
}

static void mips_machine_halt(void)
{
printk("mips_machine_halt\n");
#if defined(CONFIG_MSTAR_TITANIA3) ||   \
    defined(CONFIG_MSTAR_TITANIA4) ||   \
    defined(CONFIG_MSTAR_URANUS4) ||    \
    defined(CONFIG_MSTAR_TITANIA8) ||   \
    defined(CONFIG_MSTAR_TITANIA9) ||   \
    defined(CONFIG_MSTAR_JANUS2) ||     \
    defined(CONFIG_MSTAR_TITANIA11) ||  \
    defined(CONFIG_MSTAR_TITANIA12) ||  \
    defined(CONFIG_MSTAR_TITANIA13) ||  \
    defined(CONFIG_MSTAR_AMBER1) ||     \
    defined(CONFIG_MSTAR_AMBER2)||      \
    defined(CONFIG_MSTAR_AMBER5)||      \
    defined(CONFIG_MSTAR_KRONUS)

    // fire reset for global chip, active by password (8'h79)
    // 1. set to default first
    *(volatile unsigned long*)(REG_PM_SLEEP_BASE + 0xB8) = 0xff;    // reg_top_sw_rst
    // 2. active by password (8'h79)
    *(volatile unsigned long*)(REG_PM_SLEEP_BASE + 0xB8) = 0x79;    // reg_top_sw_rst

#endif
}

#if defined(CONFIG_MIPS_ATLAS)
static void atlas_machine_power_off(void)
{
	unsigned int __iomem *psustby_reg = ioremap(ATLAS_PSUSTBY_REG, sizeof(unsigned int));

	writew(ATLAS_GOSTBY, psustby_reg);
}
#endif

void mips_reboot_setup(void)
{
	_machine_restart = mips_machine_restart;
	_machine_halt = mips_machine_halt;
#if defined(CONFIG_MIPS_ATLAS)
	pm_power_off = atlas_machine_power_off;
#endif
#if defined(CONFIG_MIPS_MALTA) || defined(CONFIG_MIPS_SEAD)
	pm_power_off = mips_machine_halt;
#endif
}
