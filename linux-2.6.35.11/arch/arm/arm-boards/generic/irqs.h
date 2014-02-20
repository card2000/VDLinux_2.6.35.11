/*------------------------------------------------------------------------------
	Copyright (c) 2008 MStar Semiconductor, Inc.  All rights reserved.
------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------
    PROJECT: Columbus

	FILE NAME: include/asm-arm/arch-columbus/irqs.h

    DESCRIPTION:
          Head file of IO table definition

    HISTORY:
         <Date>     <Author>    <Modification Description>
        2008/07/18  Fred Cheng  Modify to define constants for power management
		2008/10/06	Fred Cheng	Modify INT_BASE_ADDR for Pioneer platform

------------------------------------------------------------------------------*/

#ifndef __ARCH_ARM_ASM_IRQS_H
#define __ARCH_ARM_ASM_IRQS_H
/*-----------------------------------------------------------------------------
    Include Files
------------------------------------------------------------------------------*/
#include <mach/platform.h>

/*------------------------------------------------------------------------------
    Constant
-------------------------------------------------------------------------------*/

#if defined(CONFIG_ARCH_PIONEER)

/* FIQ Definition */
#define INT_COLUMBUS_IR                 26
#define INT_COLUMBUS_DMA_DONE           20
#define INT_COLUMBUS_TOUCHPL_Y_FLAG     9
#define INT_COLUMBUS_IR_RC     	        8
#define INT_COLUMBUS_TIMER_2            5
#define INT_COLUMBUS_CPU0_2_CPU1        4	//for Pioneer: 8051 to ARM
#define INT_COLUMBUS_CPU1_2_CPU0        3	//for Pioneer: ARM to 8051
#define INT_COLUMBUS_WDT                2
#define INT_COLUMBUS_TIMER_1            1
#define INT_COLUMBUS_TIMER_0            0

/* IRQ Definition */
#define IRQ_BASE                        32
#define INT_COLUMBUS_FCIE               (IRQ_BASE + 30)
#define INT_COLUMBUS_EXT_IN		        (IRQ_BASE + 26)
#define INT_COLUMBUS_PM                 (IRQ_BASE + 25)
#define INT_COLUMBUS_KEYPAD             (IRQ_BASE + 24)
#define INT_COLUMBUS_RTC                (IRQ_BASE + 23)
#define INT_COLUMBUS_IIC_0              (IRQ_BASE + 22)
#define INT_COLUMBUS_JPD                (IRQ_BASE + 21)
#define INT_COLUMBUS_GOP                (IRQ_BASE + 20)
#define INT_COLUMBUS_DC	                (IRQ_BASE + 19)
#define INT_COLUMBUS_CCFL               (IRQ_BASE + 18)
#define INT_COLUMBUS_DR                 (IRQ_BASE + 17)
#define INT_COLUMBUS_TSP_TO_HK	        (IRQ_BASE + 16)
#define INT_COLUMBUS_FMRX	            (IRQ_BASE + 15)
#define INT_COLUMBUS_UART_1             (IRQ_BASE + 14)
#define INT_COLUMBUS_UART_0             (IRQ_BASE + 13)
#define INT_COLUMBUS_MPIF               (IRQ_BASE + 12)
#define INT_COLUMBUS_UTMI               (IRQ_BASE + 11)
#define INT_COLUMBUS_DISP               (IRQ_BASE + 10)
#define INT_COLUMBUS_AUDIO_AEC	        (IRQ_BASE + 9)
#define INT_COLUMBUS_NFIE               (IRQ_BASE + 8)
#define INT_COLUMBUS_UHC                (IRQ_BASE + 7)
#define INT_COLUMBUS_USB                (IRQ_BASE + 6)
#define INT_COLUMBUS_OTG                (IRQ_BASE + 5)
#define INT_COLUMBUS_AUDIO_1            (IRQ_BASE + 4)
#define INT_COLUMBUS_MVD		        (IRQ_BASE + 3)
#define INT_COLUMBUS_AUDIO_2            (IRQ_BASE + 2)
#define INT_COLUMBUS_DSP_TO_MCU         (IRQ_BASE + 1)
#define INT_COLUMBUS_UART_2             (IRQ_BASE + 0)

#elif defined(CONFIG_ARCH_COLUMBUS)

/* FIQ Definition */
#define INT_COLUMBUS_WDT                0
#define INT_COLUMBUS_TIMER_0            1
#define INT_COLUMBUS_TIMER_1            2
#define INT_COLUMBUS_TIMER_2            3
#define INT_COLUMBUS_TOUCHPL_Y_FLAG     4
#define INT_COLUMBUS_IR                 5
#define INT_COLUMBUS_DMA_DONE           6

/* IRQ Definition */
#define IRQ_BASE                        32
#define INT_COLUMBUS_PM                 (IRQ_BASE + 0)
#define INT_COLUMBUS_RTC                (IRQ_BASE + 1)
#define INT_COLUMBUS_AUDIO_2            (IRQ_BASE + 2)
#define INT_COLUMBUS_AUDIO_1            (IRQ_BASE + 3)
#define INT_COLUMBUS_GOP                (IRQ_BASE + 4)
#define INT_COLUMBUS_IPM                (IRQ_BASE + 5)
#define INT_COLUMBUS_DISP               (IRQ_BASE + 6)
#define INT_COLUMBUS_JPD                (IRQ_BASE + 7)
#define INT_COLUMBUS_UHC                (IRQ_BASE + 8)
#define INT_COLUMBUS_OTG                (IRQ_BASE + 9)
#define INT_COLUMBUS_USB                (IRQ_BASE + 10)
#define INT_COLUMBUS_UTMI               (IRQ_BASE + 11)
#define INT_COLUMBUS_MPIF               (IRQ_BASE + 12)
#define INT_COLUMBUS_UART_0             (IRQ_BASE + 13)
#define INT_COLUMBUS_NFIE               (IRQ_BASE + 14)
#define INT_COLUMBUS_FCIE               (IRQ_BASE + 15)
#define INT_COLUMBUS_IIC_0              (IRQ_BASE + 16)
#define INT_COLUMBUS_SPI_ARB_DUP_REQ    (IRQ_BASE + 17)
#define INT_COLUMBUS_SPI_ARB_CHG_ADDR   (IRQ_BASE + 18)
#define INT_COLUMBUS_DR                 (IRQ_BASE + 19)
#define INT_COLUMBUS_CCFL               (IRQ_BASE + 20)
#define INT_COLUMBUS_UART_2             (IRQ_BASE + 21)
#define INT_COLUMBUS_UART_1             (IRQ_BASE + 22)
#define INT_COLUMBUS_KEYPAD             (IRQ_BASE + 23)
#define INT_COLUMBUS_EXT_IN             (IRQ_BASE + 24)

#else
#error "Unknown Machine Type "
#endif  /* #ifdef CONFIG_ARCH_PIONEER  */

/* Max number of Interrupts */
#define NR_IRQS			64


/* Physcial address of Interrupt Base Register */
#if defined(CONFIG_ARCH_COLUMBUS)
#define INT_BASE_ADDR                   COLUMBUS_BASE_REG_INTR_PA
#elif defined(CONFIG_ARCH_PIONEER)
#define INT_BASE_ADDR                   0xa0000A00
#else
#error "Should not enter here"
#endif

/* Physcial address of FIQ */
#define FIQ_MASK_L                  (INT_BASE_ADDR + 0x00)
#define FIQ_MASK_H                  (INT_BASE_ADDR + 0x04)
#define FIQ_FORCE_L                 (INT_BASE_ADDR + 0x08)
#define FIQ_FORCE_H                 (INT_BASE_ADDR + 0x0C)
#define FIQ_CLEAR_L                 (INT_BASE_ADDR + 0x10)
#define FIQ_CLEAR_H                 (INT_BASE_ADDR + 0x14)
#define FIQ_RAW_STATUS_L            (INT_BASE_ADDR + 0x18)
#define FIQ_RAW_STATUS_H            (INT_BASE_ADDR + 0x1C)
#define FIQ_FINAL_STATUS_L          (INT_BASE_ADDR + 0x20)
#define FIQ_FINAL_STATUS_H          (INT_BASE_ADDR + 0x24)
#define FIQ_SEL_HL_TRIGGER_L        (INT_BASE_ADDR + 0x28)
#define FIQ_SEL_HL_TRIGGER_H        (INT_BASE_ADDR + 0x2C)

/* Physcial address of IRQ */
#define IRQ_MASK_L                  (INT_BASE_ADDR + 0x30)
#define IRQ_MASK_H                  (INT_BASE_ADDR + 0x34)
#define IRQ_FORCE_L                 (INT_BASE_ADDR + 0x40)
#define IRQ_FORCE_H                 (INT_BASE_ADDR + 0x44)
#define IRQ_SEL_HL_TRIGGER_L        (INT_BASE_ADDR + 0x50)
#define IRQ_SEL_HL_TRIGGER_H        (INT_BASE_ADDR + 0x54)
#define IRQ_FIQ2IRQ_RAW_STATUS_L    (INT_BASE_ADDR + 0x60)
#define IRQ_FIQ2IRQ_RAW_STATUS_H    (INT_BASE_ADDR + 0x64)
#define IRQ_RAW_STATUS_L            (INT_BASE_ADDR + 0x68)
#define IRQ_RAW_STATUS_H            (INT_BASE_ADDR + 0x6C)
#define IRQ_FIQ2IRQ_FINAL_STATUS_L  (INT_BASE_ADDR + 0x70)
#define IRQ_FIQ2IRQ_FINAL_STATUS_H  (INT_BASE_ADDR + 0x74)
#define IRQ_FINAL_STATUS_L          (INT_BASE_ADDR + 0x78)
#define IRQ_FINAL_STATUS_H          (INT_BASE_ADDR + 0x7C)

/* Physcial address of Common part */
#define FIQ2IRQOUT_L                (INT_BASE_ADDR + 0x80)
#define FIQ2IRQOUT_H                (INT_BASE_ADDR + 0x84)
#define FIQ_IDX                     (INT_BASE_ADDR + 0x88)
#define IRQ_IDX                     (INT_BASE_ADDR + 0x8C)

#if 0
typedef struct
{
    // FIQ part
    U32 FIQ_MASK_L;
    U32 FIQ_MASK_H;
    U32 FIQ_FORCE_L;
    U32 FIQ_FORCE_H;
    U32 FIQ_CLEAR_L;
    U32 FIQ_CLEAR_H;
    U32 FIQ_RAW_STATUS_L;
    U32 FIQ_RAW_STATUS_H;
    U32 FIQ_FINAL_STATUS_L;
    U32 FIQ_FINAL_STATUS_H;
    U32 FIQ_SEL_HL_TRIGGER_L;
    U32 FIQ_SEL_HL_TRIGGER_H;

    // IRQ part
    U32 IRQ_MASK_L;
    U32 IRQ_MASK_H;
    U32 spare_0;
    U32 spare_1;
    U32 IRQ_FORCE_L;
    U32 IRQ_FORCE_H;
    U32 spare_2;
    U32 spare_3;
    U32 IRQ_SEL_HL_TRIGGER_L;
    U32 IRQ_SEL_HL_TRIGGER_H;
    U32 spare_4;
    U32 spare_5;
    U32 IRQ_FIQ2IRQ_RAW_STATUS_L;
    U32 IRQ_FIQ2IRQ_RAW_STATUS_H;
    U32 IRQ_RAW_STATUS_L;
    U32 IRQ_RAW_STATUS_H;
    U32 IRQ_FIQ2IRQ_FINAL_STATUS_L;
    U32 IRQ_FIQ2IRQ_FINAL_STATUS_H;
    U32 IRQ_FINAL_STATUS_L;
    U32 IRQ_FINAL_STATUS_H;

    // Common part
    U32 FIQ2IRQOUT_L;
    U32 FIQ2IRQOUT_H;
    U32 FIQ_IDX;
    U32 IRQ_IDX;
} REG_INTR_CTRL_st, *PREG_INTR_CTRL_st;
#endif
#endif // __ARCH_ARM_ASM_IRQS_H
