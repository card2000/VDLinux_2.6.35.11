#
# Carsten Langgaard, carstenl@mips.com
# Copyright (C) 1999,2000 MIPS Technologies, Inc.  All rights reserved.
#
# Copyright (C) 2008 Wind River Systems, Inc.
#   written by Ralf Baechle <ralf@linux-mips.org>
#
obj-y				:= nt72558-amon.o nt72558-cmdline.o \
				   nt72558-display.o nt72558-init.o nt72558-int.o \
				   nt72558-memory.o nt72558-platform.o \
				   nt72558-reset.o nt72558-setup.o nt72558-time.o  \
					 nt72558-clk.o
#				   nt72558-wbflush.o

obj-$(CONFIG_EARLY_PRINTK)	+= nt72558-console.o
obj-$(CONFIG_PCI)		+= nt72558-pci.o

# FIXME FIXME FIXME
obj-$(CONFIG_MIPS_MT_SMTC)	+= nt72558-smtc.o

EXTRA_CFLAGS += -Werror
