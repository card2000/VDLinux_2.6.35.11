/*-----------------------------------------------------------------------------
 *
 * Copyright (c) 2009 MStar Semiconductor, Inc.  All rights reserved.
 *
 *-----------------------------------------------------------------------------
 * FILE
 *      arch/arm/mach-columbus/include/mach/platform.h
 *
 * DESCRIPTION
 *      Platform specific declarations.
 *
 * HISTORY
 *      <Date>      <Author>        <Modification Description>
 *      2009/11/30  Calvin Hung     Initial Revision
 *
 */


#ifndef	__PLATFORM_H__
#define	__PLATFORM_H__

//------------------------------------------------------------------------------
//  Include Files
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//  Macros
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
//  Define:  COLUMBUS_BASE_REG_RIU_PA
//
//  Locates the RIU register base.
//
#define COLUMBUS_BASE_REG_RIU_PA                        (0xA0000000)

//------------------------------------------------------------------------------
//
//  Define:  COLUMBUS_BASE_REG_OP2DISPMISC_PA
//
//  Locates the OP2 Disp Misc register base.
//
#define COLUMBUS_BASE_REG_OP2DISPMISC_PA              (0xA0003200)

//------------------------------------------------------------------------------
//
//  Define:  COLUMBUS_BASE_REG_OP2LPLL_PA
//
//  Locates the OP2 LPLL register base.
//
#define COLUMBUS_BASE_REG_OP2LPLL_PA              (0xA0003400)

//------------------------------------------------------------------------------
//
//  Define:  COLUMBUS_BASE_REG_TOUCH_PA
//
//  Locates the Touch register base.
//
#define COLUMBUS_BASE_REG_TOUCH_PA              (0xA0003700)

//------------------------------------------------------------------------------
//
//  Define:  COLUMBUS_BASE_REG_OP2DITH_PA
//
//  Locates the OP2 Dith register base.
//
#define COLUMBUS_BASE_REG_OP2DITH_PA              (0xA0003800)

//------------------------------------------------------------------------------
//
//  Define:  COLUMBUS_BASE_REG_OP2PIP_PA
//
//  Locates the OP2 PIP register base.
//
#define COLUMBUS_BASE_REG_OP2PIP_PA             (0xA0003A00)

//------------------------------------------------------------------------------
//
//  Define:  COLUMBUS_BASE_REG_CHIPTOP_PA
//
//  Locates the chiptop register base.
//
#define COLUMBUS_BASE_REG_CHIPTOP_PA            (0xA0003C00)

//------------------------------------------------------------------------------
//
//  Define:  COLUMBUS_BASE_REG_USBC_PA
//
//  Locates the usb Controller register base.
//
#define COLUMBUS_BASE_REG_USBC_PA               (0xA0004A00)

//------------------------------------------------------------------------------
//
//  Define:  COLUMBUS_BASE_REG_USBOTG_PA
//
//  Locates the usb OTG register base.
//
#define COLUMBUS_BASE_REG_USBOTG_PA             (0xA0004C00)

//------------------------------------------------------------------------------
//
//  Define:  COLUMBUS_BASE_REG_USBUTMI_PA
//
//  Locates the usb UTMI register base.
//
#define COLUMBUS_BASE_REG_USBUTMI_PA            (0xA0003E00)

//------------------------------------------------------------------------------
//
//  Define:  COLUMBUS_BASE_REG_PMU_PA
//
//  Locates the PWM register base.
//
#define COLUMBUS_BASE_REG_PWM_PA                (0xA0006880)

//------------------------------------------------------------------------------
//
//  Define:  COLUMBUS_BASE_REG_INTR_PA
//
//  Locates the interrupt controller register base.
//
#define COLUMBUS_BASE_REG_INTR_PA               (0xA0005600)

//------------------------------------------------------------------------------
//
//  Define:  COLUMBUS_BASE_REG_SCVOP_PA
//
//  Locates the Scalar VOP register base.
//
#define COLUMBUS_BASE_REG_SCVOP_PA              (0xA0006200)

//------------------------------------------------------------------------------
//
//  Define:  COLUMBUS_BASE_REG_I2C_PA
//
//  Locates the I2C register base.
//
#define COLUMBUS_BASE_REG_I2C_PA                (0xA0006840)

//------------------------------------------------------------------------------
//
//  Define:  COLUMBUS_BASE_REG_RTC_PA
//
//  Locates the RTC register base.
//
#define COLUMBUS_BASE_REG_RTC_PA                (0xA0006900)

//------------------------------------------------------------------------------
//
//  Define:  COLUMBUS_BASE_REG_GOP_PA
//
//  Locates the GOP register base.
//
#define COLUMBUS_BASE_REG_GOP_PA                (0xA0006A00)

//------------------------------------------------------------------------------
//
//  Define:  COLUMBUS_BASE_REG_GE_PA
//
//  Locates the GE register base.
//
#define COLUMBUS_BASE_REG_GE_PA                    (0xA0006C00)

//------------------------------------------------------------------------------
//
//  Define:  COLUMBUS_BASE_REG_JPD_PA
//
//  Locates the JPD register base.
//
#define COLUMBUS_BASE_REG_JPD_PA                (0xA0006E00)

//------------------------------------------------------------------------------
//
//  Define:  COLUMBUS_BASE_REG_SARADC
//
//  Locates the SAR ADC register base.
//
#define COLUMBUS_BASE_REG_SARADC_PA             (0xA0007400)

//------------------------------------------------------------------------------
//
//  Define:  COLUMBUS_BASE_REG_UART0~2_PA
//
//  Locates the uart 0~2 register base.
//
#define COLUMBUS_BASE_REG_UART0_PA              (0xA0007600)
#define COLUMBUS_BASE_REG_UART1_PA              (0xA0007680)
#define COLUMBUS_BASE_REG_UART2_PA              (0xA0007700)

#define COLUMBUS_UART_ENABLE_REG_PA             (COLUMBUS_BASE_REG_CHIPTOP_PA+(8<<2))

//------------------------------------------------------------------------------
//
//  Define:  COLUMBUS_BASE_REG_WATCHDOG_PA
//
//  Locates the watchdog register base.
//
#define COLUMBUS_BASE_REG_WATCHDOG_PA           (0xA00078C0)

//------------------------------------------------------------------------------
//
//  Define:  COLUMBUS_BASE_REG_TIMER_PA
//
//  Locates the timer register base.
//
#define COLUMBUS_BASE_REG_TIMER0_PA              (0xA0007900)
#define COLUMBUS_BASE_REG_TIMER1_PA              (0xA0007940)
#define COLUMBUS_BASE_REG_TIMER2_PA              (0xA0007780)

//------------------------------------------------------------------------------
//
//  Define:  COLUMBUS_BASE_REG_G3D_PA
//
//  Locates the G3D register base.
//
#define COLUMBUS_BASE_REG_G3D_PA                (0xA0007C00)

//-----------------------------------------------------------------------------
//
//  Define:  COLUMBUS_BASE_REG_PMU_PA
//
//  Locates the PMU register base.
//
#define COLUMBUS_BASE_REG_PMU_PA                (0xA0006940)

//------------------------------------------------------------------------------
//
//  Define:  COLUMBUS_BASE_REG_PWM_PA
//
//  Locates the PWM register base.
//
#define COLUMBUS_BASE_REG_PWM_PA                (0xA0006880)

//------------------------------------------------------------------------------
//
//  Define:  COLUMBUS_BASE_REG_MPIF_PA
//
//  Locates the MPIF register base.
//
#define COLUMBUS_BASE_REG_MPIF_PA                (0xA0000C00)
#define COLUMBUS_BASE_DMA_MPIF_PA                (0x08086000)   // 2009/0204 Chun 64KB MPIF DMA supported from V1.0015
#define COLUMBUS_BASE_REG_UTMI_PA                (0xA0003E00)   // temporary definition for opening the USB MPLL

//------------------------------------------------------------------------------
//
//  Define:  COLUMBUS_BASE_REG_ARM_PA
//
//  Locates the ARM register base.
//
#define COLUMBUS_BASE_REG_ARM_PA                 (0xA0001E00)

//-----------------------------------------------------------------------------

// Chip revisions
#define COLUMBUS_REVISION_U01    (0x0)
#define COLUMBUS_REVISION_U02    (0x1)
#define COLUMBUS_REVISION_U03    (0x2)

//------------------------------------------------------------------------------
//
//  Macros:  TYPE_CAST
//
//  This macros interprets the logic of type casting and shows the old type and
//  the new type of the casted variable.
//
#define TYPE_CAST(OldType, NewType, Var)    ((NewType)(Var))

//------------------------------------------------------------------------------
//
//  Macros:  MSTAR_ASSERT
//
//  This macro implements the assertion no matter in Debug or Release build.
//
#define MSTAR_ASSERT_PRINT(exp,file,line) printk(("\r\n*** ASSERTION FAILED in ") (file) ("(") (#line) ("):\r\n") (#exp) ("\r\n"))
#define MSTAR_ASSERT_AT(exp,file,line) (void)( (exp) || (MSTAR_ASSERT_PRINT(exp,file,line), 0 ) )
#define MSTAR_ASSERT(exp) MSTAR_ASSERT_AT(exp,__FILE__,__LINE__)

#endif // __PLATFORM_H__

/* 	END */
