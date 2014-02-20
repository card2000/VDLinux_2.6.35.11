/*
 * Copyright (C) 2011  Novatek , Inc.
 *	All rights reserved.
 */

#include <linux/module.h>
#include <linux/spinlock.h>

#include <asm/mips-boards/nvt72558.h>

static spinlock_t sys_mpll_lock = SPIN_LOCK_UNLOCKED;

/*
 * Get System MPLL 
 */
unsigned long get_sys_mpll(unsigned long offset)
{
	unsigned long rVal;
	unsigned long flags;

	spin_lock_irqsave(&sys_mpll_lock, flags);

	SYS_MPLL_PBUS_PAGE_ENABLE = _SYS_MPLL_PAGE_B_ENABLE;
	SYS_MPLL_PBUS_OFFSET = offset;
	rVal = SYS_MPLL_PBUS_RD_DATA;
	SYS_MPLL_PBUS_OFFSET = (offset + 1);
	rVal |= (SYS_MPLL_PBUS_RD_DATA << 8);
	SYS_MPLL_PBUS_OFFSET = (offset + 2);
	rVal |= (SYS_MPLL_PBUS_RD_DATA << 16);

	spin_unlock_irqrestore(&sys_mpll_lock, flags);

	rVal *= 12;
	rVal >>= 17;
	rVal *= 1000;

	return rVal;
}

/*
 * Set System MPLL
 */
void set_sys_mpll(unsigned long offset, unsigned long value)
{
	unsigned long flags;

	spin_lock_irqsave(&sys_mpll_lock, flags);

	SYS_MPLL_PBUS_PAGE_ENABLE = _SYS_MPLL_PAGE_B_ENABLE;
	SYS_MPLL_PBUS_OFFSET = offset;
	SYS_MPLL_PBUS_WR_DATA = (value & 0xff);
	SYS_MPLL_PBUS_OFFSET = (offset + 1);
	SYS_MPLL_PBUS_WR_DATA = ((value >> 8) & 0xff);
	SYS_MPLL_PBUS_OFFSET = (offset + 2);
	SYS_MPLL_PBUS_WR_DATA = ((value >> 16) & 0xff);

	spin_unlock_irqrestore(&sys_mpll_lock, flags);
}

EXPORT_SYMBOL(get_sys_mpll);
EXPORT_SYMBOL(set_sys_mpll);

