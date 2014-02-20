
#ifndef _EHCI_MSTAR_H
#define _EHCI_MSTAR_H

#if defined(CONFIG_MSTAR_URANUS4) || defined(CONFIG_MSTAR_AMBER5) || defined(CONFIG_MSTAR_AMBER3) 
#define ENABLE_THIRD_EHC
#endif

#if defined(CONFIG_MSTAR_TITANIA) || defined (CONFIG_MSTAR_TITANIA2)
#define _MapBase_nonPM_Euclid 0xbf800000
#elif defined(CONFIG_ARM)
#define _MapBase_nonPM_Euclid 0xfd200000

#define MIU0_BUS_BASE_ADDR	((unsigned long)0x40000000)
#define MIU0_PHY_BASE_ADDR	((unsigned long)0x00000000)
/* MIU0 1.5G*/
#define MIU0_SIZE		((unsigned long)0x60000000)

#define MIU1_BUS_BASE_ADDR	((unsigned long)0xA0000000)
#define MIU1_PHY_BASE_ADDR	((unsigned long)0x80000000)
/* MIU1 1G*/
#define MIU1_SIZE		((unsigned long)0x40000000)

#define BUS2PA(A)	\
	(((A>=MIU0_BUS_BASE_ADDR)&&(A<(MIU0_BUS_BASE_ADDR+MIU0_SIZE)))?	\
		(A-MIU0_BUS_BASE_ADDR+MIU0_PHY_BASE_ADDR):	\
		(((A>= MIU1_BUS_BASE_ADDR)&&(A<MIU1_BUS_BASE_ADDR+MIU1_SIZE))?	\
			(A-MIU1_BUS_BASE_ADDR+MIU1_PHY_BASE_ADDR):	\
			(0xFFFFFFFF)))

#define PA2BUS(A)	\
	(((A>=MIU0_PHY_BASE_ADDR)&&(A<(MIU0_PHY_BASE_ADDR+MIU0_SIZE)))?	\
		(A-MIU0_PHY_BASE_ADDR+MIU0_BUS_BASE_ADDR):	\
		(((A>= MIU1_PHY_BASE_ADDR)&&(A<MIU1_PHY_BASE_ADDR+MIU1_SIZE))?	\
			(A-MIU1_PHY_BASE_ADDR+MIU1_BUS_BASE_ADDR):	\
			(0xFFFFFFFF)))
#else
#define _MapBase_nonPM_Euclid 0xbf200000
#endif

#define _MSTAR_USB_BASEADR    _MapBase_nonPM_Euclid

#define _MSTAR_UTMI0_BASE  (_MSTAR_USB_BASEADR+0x7500)
#define _MSTAR_UHC0_BASE   (_MSTAR_USB_BASEADR+0x4800)
#define _MSTAR_USBC0_BASE  (_MSTAR_USB_BASEADR+0x0E00)

#define _MSTAR_UTMI1_BASE  (_MSTAR_USB_BASEADR+0x7400)
#define _MSTAR_UHC1_BASE   (_MSTAR_USB_BASEADR+0x1A00)
#define _MSTAR_USBC1_BASE  (_MSTAR_USB_BASEADR+0x0F00)

#ifdef ENABLE_THIRD_EHC
#if defined(CONFIG_MSTAR_URANUS4)
#define _MSTAR_UTMI2_BASE  (_MSTAR_USB_BASEADR+(0x2A00*2))
#define _MSTAR_UHC2_BASE   (_MSTAR_USB_BASEADR+(0x10300*2))
#define _MSTAR_USBC2_BASE  (_MSTAR_USB_BASEADR+(0x10200*2))
#endif
#if defined(CONFIG_MSTAR_TITANIA12) || defined(CONFIG_MSTAR_AMBER5) || defined(CONFIG_MSTAR_AMBER3) 
#define _MSTAR_UTMI2_BASE  (_MSTAR_USB_BASEADR+(0x3900*2))
#define _MSTAR_UHC2_BASE   (_MSTAR_USB_BASEADR+(0x13900*2))
#define _MSTAR_USBC2_BASE  (_MSTAR_USB_BASEADR+(0x13800*2))
#endif
#endif

#define DISABLE         0
#define ENABLE          1

#define BIT0  0x01
#define BIT1  0x02
#define BIT2  0x04
#define BIT3  0x08
#define BIT4  0x10
#define BIT5  0x20
#define BIT6  0x40
#define BIT7  0x80

#if defined(CONFIG_MSTAR_TITANIA3) || defined(CONFIG_MSTAR_TITANIA8) || defined(CONFIG_MSTAR_TITANIA9) || defined(CONFIG_MSTAR_URANUS4) || defined(CONFIG_MSTAR_AMBER1) || defined(CONFIG_MSTAR_AMBER5) || defined(CONFIG_MSTAR_AMBER3)
#define _USB_T3_WBTIMEOUT_PATCH 1
#else
#define _USB_T3_WBTIMEOUT_PATCH 0
#endif

#if defined(CONFIG_MSTAR_AMBER3)
#define _USB_A3_SHORT_PACKET_LOSE_INT_PATCH 1
#else
#define _USB_A3_SHORT_PACKET_LOSE_INT_PATCH 0
#endif

#if defined(CONFIG_MSTAR_TITANIA8) || defined(CONFIG_MSTAR_TITANIA9) || defined(CONFIG_MSTAR_JANUS2) || defined(CONFIG_MSTAR_AMBER1) || defined(CONFIG_MSTAR_AMBER5) || defined(CONFIG_MSTAR_AMBER3)
#define _USB_128_ALIGMENT 1
#else
#define _USB_128_ALIGMENT 0
#endif
//-----------------------------------------
// Titania3_series_start_ehc flag:
#define EHCFLAG_NONE         0
#define EHCFLAG_DPDM_SWAP    1
#define EHCFLAG_TESTPKG      2


#endif	/* _EHCI_MSTAR_H */

