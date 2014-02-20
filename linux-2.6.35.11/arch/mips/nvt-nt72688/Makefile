#
# Carsten Langgaard, carstenl@mips.com
# Copyright (C) 1999,2000 MIPS Technologies, Inc.  All rights reserved.
#
# Copyright (C) 2008 Wind River Systems, Inc.
#   written by Ralf Baechle <ralf@linux-mips.org>
#
obj-y				:= nt72688-amon.o nt72688-cmdline.o \
				   nt72688-display.o nt72688-init.o nt72688-int.o \
				   nt72688-memory.o nt72688-platform.o \
				   nt72688-reset.o nt72688-setup.o nt72688-time.o # \
#				   nt72688-wbflush.o

obj-$(CONFIG_EARLY_PRINTK)	+= nt72688-console.o
obj-$(CONFIG_PCI)		+= nt72688-pci.o

# FIXME FIXME FIXME
obj-$(CONFIG_MIPS_MT_SMTC)	+= nt72688-smtc.o

EXTRA_CFLAGS += -Werror
