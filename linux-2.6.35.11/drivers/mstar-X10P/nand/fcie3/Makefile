#
# Makefile for MStar NAND device drivers.
#

# general options
EXTRA_CFLAGS      +=  -Idrivers/mstar/include
EXTRA_CFLAGS      +=  -Idrivers/mstar/nand

# specific options
EXTRA_CFLAGS      +=  -DRED_LION

# files
obj-y		+= src/drvNand_platform.o
obj-y		+= src/drvNand_Linux.o
obj-y		+= src/drvNand_hal.o
obj-y		+= src/drvNand.o

