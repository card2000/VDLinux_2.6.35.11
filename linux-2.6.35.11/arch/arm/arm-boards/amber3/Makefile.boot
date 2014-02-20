#if((CONFIG_ARCH_COLUMBUS),y)
#ZTEXTADDR:=0x00008000
#endif
#if((CONFIG_ARCH_PIONEER),y)
#ZTEXTADDR:=0x00008000
#endif
zreladdr-y   := 0x0a008000
#params_phys-y   := 0x09000000
#initrd_phys-y   := 0x20410000

