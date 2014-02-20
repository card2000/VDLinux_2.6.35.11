#include <linux/kernel.h>

#define CONFIG_HW_SHA1 0

//#define SECURE_DEBUG

#define CIP_CRIT_PRINT(fmt, ...)		printk(KERN_CRIT "[CIP_KERNEL] " fmt,##__VA_ARGS__)
#define CIP_WARN_PRINT(fmt, ...)		printk(KERN_WARNING "[CIP_KERNEL] " fmt,##__VA_ARGS__)
#define CIP_DEBUG_PRINT(fmt, ...)		printk(KERN_DEBUG "[CIP_KERNEL] " fmt,##__VA_ARGS__)

#ifndef	SIZE_1K
#define	SIZE_1K					(1024)
#endif

#ifndef	SZ_1M	
#define	SZ_1M                   (1048576)           // 1024*1024
#endif

#ifndef SZ_4K
#define SZ_4K                   (4096)              // 4*1024
#endif

#ifndef	SZ_128K
#define SZ_128K                 (131072)            // 128*1024
#endif

#ifndef	SZ_256K
#define SZ_256K                 (262144)            // 256*1024
#endif

#ifndef	SZ_4M
#define SZ_4M                   (4194304)           // 4*1024*1024
#endif

#define HMAC_SIZE			(20)

#define RSA_1024_SIGN_SIZE  (128)
#define SLEEP_WAITING       (10)


#ifdef NT72688	// Novatek NT72688 : FIXME 
#define ROOTFS_PARTITION	("/dev/mmcblk0p3")
#define MICOM_DEV			/dev/ttyS1  /* NOTE : Do not quote this string */
#endif

#ifdef CONFIG_NVT_NT72558 // NVT558 
#define ROOTFS_PARTITION1    ("/dev/bml0/6")
#define ROOTFS_PARTITION2    ("/dev/bml0/8")
#endif

#ifdef CONFIG_MSTAR_AMBER5 // X10
#define ROOTFS_PARTITION	("/dev/mmcblk0p3")
#define MICOM_DEV			/dev/ttyS1  /* NOTE : Do not quote this string */
#endif

#ifdef CONFIG_ARCH_SDP1103 //ECHOS
#define ROOTFS_PARTITION	("/dev/mmcblk0p3")
#define MICOM_DEV			/dev/ttyS0  /* NOTE : Do not quote this string */
#endif

#if defined(CONFIG_ARCH_SDP1102) || defined(CONFIG_ARCH_SDP1002) //ECHOP
#define ROOTFS_PARTITION	("/dev/mmcblk0p3")
#define MICOM_DEV			/dev/ttyS1  /* NOTE : Do not quote this string */
#endif

#if defined(CONFIG_ARCH_SDP1106) ||defined(CONFIG_ARCH_SDP1004) //ECHOP
#define ROOTFS_PARTITION	("/dev/mmcblk0p3")
#define MICOM_DEV			/dev/ttyS0  /* NOTE : Do not quote this string */
#endif

#if defined(CONFIG_ARCH_SDP1105) //ECHOB
#define ROOTFS_PARTITION        ("/dev/mmcblk0p5")
#define MICOM_DEV                       /dev/ttyS0  /* NOTE : Do not quote this string */
#endif



#define SIGNAL_FLAG_TEST    "/dtv/.data_test"

typedef struct 
{
	unsigned char au8PreCmacResult[RSA_1024_SIGN_SIZE];	// MAC
	unsigned int msgLen;	// the length of a message to be authenticated
} MacInfo_t;

typedef struct 
{
	MacInfo_t macAuthULD;
} macAuthULd_t;

int getAuthUld(macAuthULd_t *authuld);
int read_from_file_or_ff (int fd, unsigned char *buf, int size);
int verify_rsa_signature(unsigned char *pText, unsigned long pTextLen, unsigned char *signature, unsigned long signLen);

#ifdef	SECURE_DEBUG
void print_20byte(unsigned char *bytes);
void print_128byte(unsigned char *bytes);
#endif
