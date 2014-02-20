/*
 *  linux/drivers/char/8250.h
 *
 *  Driver for 8250/16550-type serial ports
 *
 *  Based on drivers/char/serial.c, by Linus Torvalds, Theodore Ts'o.
 *
 *  Copyright (C) 2001 Russell King.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/serial_8250.h>

#define MBX_CTRL1 0x0E5E              //total bytes
/*
   total bytes      byte has been write
   7 6 5 4           3 2 1 0
 */
#define MBX_CTRL2 0x0E5F
/*
   bytes has been received      mips write indicater  51 write indicater   Indicate write finish  indicate data has been read
   7 6 5 4                               3                            2                         1                            0
 */

#define MBX_REGISTER_1 0x0E6C
#define MBX_REGISTER_2 0x0E5D
#define MBX_REGISTER_3 0x0E70
#define MBX_REGISTER_4 0x0E71
#define MBX_REGISTER_5 0x0E72
#define MBX_REGISTER_6 0x0E73
#define MBX_REGISTER_8 0x1033E1
#define MBX_REGISTER_9 0x1033E0
#define MBX_MASK_TOTAL 0xF0     //init 0
#define MBX_MASK_WRITED 0x0F // init 0
#define MBX_MASK_RECEIVED 0xF0 // init 0
#define MBX_MASK_MIPS_W 0x08 // init 0
#define MBX_MASK_51_W 0x04    // init 0
#define MBX_MASK_FINISH 0x02 //init 0
#define MBX_MASK_DATA_READED 0x01  // init to 1

#define MBX_REGISTER_COUNT 6
struct old_serial_port {
	unsigned int uart;
	unsigned int baud_base;
	unsigned int port;
	unsigned int irq;
	unsigned int flags;
	unsigned char hub6;
	unsigned char io_type;
	unsigned char *iomem_base;
	unsigned short iomem_reg_shift;
};

/*
 * This replaces serial_uart_config in include/linux/serial.h
 */
struct serial8250_config {
	const char	*name;
	unsigned short	fifo_size;
	unsigned short	tx_loadsz;
	unsigned char	fcr;
	unsigned int	flags;
};

#define UART_CAP_FIFO	(1 << 8)	/* UART has FIFO */
#define UART_CAP_EFR	(1 << 9)	/* UART has EFR */
#define UART_CAP_SLEEP	(1 << 10)	/* UART has IER sleep */
#define UART_CAP_AFE	(1 << 11)	/* MCR-based hw flow control */
#define UART_CAP_UUE	(1 << 12)	/* UART needs IER bit 6 set (Xscale) */

#define UART_BUG_QUOT	(1 << 0)	/* UART has buggy quot LSB */
#define UART_BUG_TXEN	(1 << 1)	/* UART has buggy TX IIR status */
#define UART_BUG_NOMSR	(1 << 2)	/* UART has buggy MSR status bits (Au1x00) */
#define UART_BUG_THRE	(1 << 3)	/* UART has buggy THRE reassertion */

#define PROBE_RSA	(1 << 0)
#define PROBE_ANY	(~0)

#define HIGH_BITS_OFFSET ((sizeof(long)-sizeof(int))*8)

#ifdef CONFIG_SERIAL_8250_SHARE_IRQ
#define SERIAL8250_SHARE_IRQS 1
#else
#define SERIAL8250_SHARE_IRQS 0
#endif

#if defined(__alpha__) && !defined(CONFIG_PCI)
/*
 * Digital did something really horribly wrong with the OUT1 and OUT2
 * lines on at least some ALPHA's.  The failure mode is that if either
 * is cleared, the machine locks up with endless interrupts.
 */
#define ALPHA_KLUDGE_MCR  (UART_MCR_OUT2 | UART_MCR_OUT1)
#elif defined(CONFIG_SBC8560)
/*
 * WindRiver did something similarly broken on their SBC8560 board. The
 * UART tristates its IRQ output while OUT2 is clear, but they pulled
 * the interrupt line _up_ instead of down, so if we register the IRQ
 * while the UART is in that state, we die in an IRQ storm. */
#define ALPHA_KLUDGE_MCR (UART_MCR_OUT2)
#else
#define ALPHA_KLUDGE_MCR 0
#endif
