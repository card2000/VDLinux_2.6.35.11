/*------------------------------------------------------------------------------
	Copyright (c) 2008 MStar Semiconductor, Inc.  All rights reserved.
------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------
    PROJECT: Columbus

	FILE NAME: include/asm-arm/arch-columbus/io.h

    DESCRIPTION:
          Head file of IO table definition

    HISTORY:
         <Date>     <Author>    <Modification Description>
        2008/07/18  Fred Cheng  Modify to add ITCM and DTCM'd IO tables

------------------------------------------------------------------------------*/

#ifndef __ASM_ARM_ARCH_IO_H
#define __ASM_ARM_ARCH_IO_H

/*------------------------------------------------------------------------------
    Constant
-------------------------------------------------------------------------------*/

/* max of IO Space */
#define IO_SPACE_LIMIT 0xffffffff

/* Constants of Columbus RIU */
#define IO_PHYS         0xa0000000
#define IO_OFFSET       0x40000000
#define IO_SIZE         0x00400000
#define IO_VIRT         (IO_PHYS + IO_OFFSET)
#define io_p2v(pa)      ((pa) + IO_OFFSET)
#define io_v2p(va)      ((va) - IO_OFFSET)
#define IO_ADDRESS(x)   io_p2v(x)

/* Constants of Columbus ITCM */
#define COLUMBUS_ITCM_PHYS      0x00000000
#define COLUMBUS_ITCM_OFFSET    0xf0000000
#define COLUMBUS_ITCM_SIZE      0x1000
#define COLUMBUS_ITCM_VIRT      (COLUMBUS_ITCM_PHYS + COLUMBUS_ITCM_OFFSET)
#define itcm_p2v(pa)            ((pa) + COLUMBUS_ITCM_OFFSET)
#define itcm_v2p(va)            ((va) - COLUMBUS_ITCM_OFFSET)
#define ITCM_ADDRESS(x)         itcm_p2v(x)

/* Constants of Columbus DTCM */
#define COLUMBUS_DTCM_PHYS      0x00004000
#define COLUMBUS_DTCM_OFFSET    0xf0000000
#define COLUMBUS_DTCM_SIZE      0x1000
#define COLUMBUS_DTCM_VIRT      (COLUMBUS_DTCM_PHYS + COLUMBUS_DTCM_OFFSET)
#define dtcm_p2v(pa)            ((pa) + COLUMBUS_DTCM_OFFSET)
#define dtcm_v2p(va)            ((va) - COLUMBUS_DTCM_OFFSET)
#define DTCM_ADDRESS(x)         dtcm_p2v(x)

/* Constants of 8051 memory device*/
#define COLUMBUS_MEM51_PHY      0x08000000
#define COLUMBUS_MEM51_OFFSET   0xe0000000
#define COLUMBUS_MEM51_SIZE     0x02000000
#define COLUMBUS_MEM51_VIRT     (COLUMBUS_MEM51_PHY + COLUMBUS_MEM51_OFFSET)
#define mem51_p2v(pa)           ((pa) + COLUMBUS_MEM51_OFFSET)
#define mem51_v2p(va)           ((va) - COLUMBUS_MEM51_OFFSET)
#define MEM51_ADDRESS(x)         mem51_p2v(x)


/*------------------------------------------------------------------------------
    Macro
-------------------------------------------------------------------------------*/
/* macro for __iomem */
static inline void __iomem *__io(unsigned long addr)
{
	return (void __iomem *)addr;
}
#define __io(a)	__io(a)
#define __mem_pci(a) (a)

/* read register by byte */
#define columbus_readb(a) (*(volatile unsigned char *)IO_ADDRESS(a))

/* read register by word */
#define columbus_readw(a) (*(volatile unsigned short *)IO_ADDRESS(a))

/* read register by long */
#define columbus_readl(a) (*(volatile unsigned int *)IO_ADDRESS(a))

/* write register by byte */
#define columbus_writeb(v,a) (*(volatile unsigned char *)IO_ADDRESS(a) = (v))

/* write register by word */
#define columbus_writew(v,a) (*(volatile unsigned short *)IO_ADDRESS(a) = (v))

/* write register by long */
#define columbus_writel(v,a) (*(volatile unsigned int *)IO_ADDRESS(a) = (v))


//------------------------------------------------------------------------------
//
//  Macros:  INREGx/OUTREGx/SETREGx/CLRREGx
//
//  This macros encapsulates basic I/O operations.
//  Memory address space operation is used on all platforms.
//
#define INREG8(x)           columbus_readb(x)
#define OUTREG8(x, y)       columbus_writeb((u8)(y), x)
#define SETREG8(x, y)       OUTREG8(x, INREG8(x)|(y))
#define CLRREG8(x, y)       OUTREG8(x, INREG8(x)&~(y))

#define INREG16(x)          columbus_readw(x)
#define OUTREG16(x, y)      columbus_writew((u16)(y), x)
#define SETREG16(x, y)      OUTREG16(x, INREG16(x)|(y))
#define CLRREG16(x, y)      OUTREG16(x, INREG16(x)&~(y))

#define INREG32(x)          columbus_readl(x)
#define OUTREG32(x, y)      columbus_writel((u32)(y), x)
#define SETREG32(x, y)      OUTREG32(x, INREG32(x)|(y))
#define CLRREG32(x, y)      OUTREG32(x, INREG32(x)&~(y))

#define BIT_0       (1<<0)
#define BIT_1       (1<<1)
#define BIT_2       (1<<2)
#define BIT_3       (1<<3)
#define BIT_4       (1<<4)
#define BIT_5       (1<<5)
#define BIT_6       (1<<6)
#define BIT_7       (1<<7)
#define BIT_8       (1<<8)
#define BIT_9       (1<<9)
#define BIT_10      (1<<10)
#define BIT_11      (1<<11)
#define BIT_12      (1<<12)
#define BIT_13      (1<<13)
#define BIT_14      (1<<14)
#define BIT_15      (1<<15)


//------------------------------------------------------------------------------
//
//  Function: read_chip_revision
//
//  This inline function returns the chip revision (for drivers)
//
static inline u32 read_chip_revision(void)
{
	u32 u32_rev = *((volatile u32*)(0xA0003C00+(0x67<<2)));
    return ((u32_rev & 0xff00)>>8);
}




#endif /* __ASM_ARM_ARCH_IO_H */

