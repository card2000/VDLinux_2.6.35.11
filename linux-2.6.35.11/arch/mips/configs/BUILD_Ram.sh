#!/bin/sh
echo "1. copy config"
cp arch/mips/configs/NT72558_defconfig_debug .config
echo "2. make oldconfig"
make oldconfig
echo "3. kernel compile"
make vmlinux.bin -j 4
echo "4. module compile"
make modules

if [ -e ./KO ]
then
	rm -rf KO
fi
mkdir KO
cp ./drivers/fsr/fsr.ko KO/.
cp ./drivers/fsr/fsr_stl.ko KO/.
cp ./fs/rfs/rfs_fat.ko KO/.
cp ./fs/rfs/rfs_glue.ko KO/.
cp ./drivers/scsi/scsi_wait_scan.ko KO/.
cp ./drivers/usb/core/usbcore.ko KO/.
cp ./drivers/usb/host/ehci-hcd.ko KO/.
cp ./drivers/usb/storage/usb-storage.ko KO/.

mipsel-linux-gnu-strip -g KO/*
sudo rm ../rootfs-vdlinux/lib/modules/*
cp KO/* ../rootfs-vdlinux/lib/modules/.

echo "5. kernel compile"
make vmlinux.bin -j 4

echo "6. rename kernel, rootfs...."
mv vmlinux.img uImage
mv initramfs.bin rootfs.img

