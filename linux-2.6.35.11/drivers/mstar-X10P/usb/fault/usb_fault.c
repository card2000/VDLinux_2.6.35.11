/* Change Logs
   2010-03-04 : ver 1.0 : Mstar usb fault driver using GPIO port
   2010-03-10 : ver 1.1 : cdev_alloc support by code review, support suspend / resume at Device Manager
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/kthread.h>
#include "mdrv_types.h"

#define MSLEEP	500
#define USE_EXT_DEFAULT	LEVEL_LOW

#define RST_UNABLE_TO_READ	0
#define RST_READY_TO_READ	1
#define RST_SKIP_READ		2

#define USB_FAULT_MAJOR 234
#define MODULE_NAME "usbfault"

#define USBFAULT_BLOCKED_READ	0
#define USBFAULT_UNBLOCKED_READ	1
#define USBFAULT_READ_DISABLE	2
#define USBFAULT_READ_ENABLE	3

#define DRV_DEBUG 0 // 1 - enabled, 0 - disabled
#define LOG(a...) if (DRV_DEBUG){ \
				printk("[USB_FAULT:%d] ", __LINE__);\
				printk(a);\
				printk("\n");\
			   }

static dev_t chardev;
static struct cdev  *chdev;

unsigned int usb_fault = 0;
struct task_struct *usb_fault_tsk;
unsigned int usbfault_running = 0;

static ssize_t usbfault_read_blocked (struct file *, char  *, size_t, loff_t *);
static ssize_t usbfault_write (struct file *, const char *, size_t, loff_t *);
static int usbfault_open(struct inode *inode, struct file *file);
static int usbfault_close(struct inode *inode, struct file *file);
static int usbfault_ioctl (struct inode *, struct file *, unsigned int, unsigned long);
static int get_usb_fault (void);

/*
 * Mstar Specific Code
*/
#define REG_MIPS_BASE              0xBF000000 //Use 8 bit addressing 
#define MHal_GPIO_REG(addr)        (*(volatile U8*)(REG_MIPS_BASE + ( ((addr) & ~1) << 1 ) + (addr & 1) ))
//#define BIT0	                    0x00000001
//#define BIT3	                    0x00000008

/*
=======================================
GPIO6 (ball J5) connect to USB0_OCD
=======================================
*/

void Set_USB0_OCD_IN(void) 
{     
	MHal_GPIO_REG(0x0E1E) |= BIT0; 
}

void Set_USB0_OCD_OUT(void) 
{     
	MHal_GPIO_REG(0x0E1E) &= ~BIT0; 
}

U8 Get_USB0_OCD_Level(void) 
{     
	U8 u8RetGPIO;     
	u8RetGPIO = MHal_GPIO_REG(0x0E22);     

	return (u8RetGPIO &= BIT0); 
}

void Set_USB0_OCD_High(void) 
{     
	MHal_GPIO_REG(0x0E20) |= BIT0; 
}

void Set_USB0_OCD_Low(void) 
{     
	MHal_GPIO_REG(0x0E20) &= ~BIT0; 
}

/*
=======================================
GPIO17 (ball J4) connect to USB1_OCD
=======================================
*/

void Set_USB1_OCD_IN(void) 
{     
	MHal_GPIO_REG(0x0E1F) |= BIT3; 
}

void Set_USB1_OCD_OUT(void) 
{     
	MHal_GPIO_REG(0x0E1F) &= ~BIT3; 
}

U8 Get_USB1_OCD_Level(void) 
{     
	U8 u8RetGPIO;     
	u8RetGPIO = MHal_GPIO_REG(0x0E23);     
	u8RetGPIO = (u8RetGPIO>>3);     

	return (u8RetGPIO &= BIT0); 
}

void Set_USB1_OCD_High(void) 
{     
	MHal_GPIO_REG(0x0E21) |= BIT3; 
}

void Set_USB1_OCD_Low(void) 
{
	MHal_GPIO_REG(0x0E21) &= ~BIT3; 
}

// End of Mstar Setting

void fault_status(void)
{
	LOG("------------------------");
	LOG("USB 0 0E1E : 0x%04X 0E20 : 0x%04X 0E22 : 0x%04X", MHal_GPIO_REG(0x0E1E), MHal_GPIO_REG(0x0E20), MHal_GPIO_REG(0x0E22) );
	LOG("USB 0 0E1F : 0x%04X 0E21 : 0x%04X 0E23 : 0x%04X", MHal_GPIO_REG(0x0E1F), MHal_GPIO_REG(0x0E21), MHal_GPIO_REG(0x0E23) );
	LOG("------------------------\n");
}

static int usb_fault_run(void *arg) 
{
	int stat0 = 0;
	int stat1 = 0;

	printk("[USB_FAULT: registered usb fault driver] \n");

	do {
		if ( kthread_should_stop() )     
		{
			printk("kthread_should_stop returns true.\n");
			break;            
		}

		// Check Period
		msleep(MSLEEP);

		// Get current USB status
		stat0 = Get_USB0_OCD_Level();
		stat1 = Get_USB1_OCD_Level();

		// Both stat0 and stat1 are HIGH
		if( stat0 && stat1 ) 			
		{
			usb_fault = 0;
			LOG("USB Over-Current is not detected. usb_fault : 0x%x \n", usb_fault);
			fault_status();
		}
		// When stat0 is LOW
		else if( stat1 )  		
		{
			usb_fault = 0x1;
			LOG("USB 0 Over-Current is detected. usb_fault : 0x%x \n", usb_fault);
			fault_status();
		}
		// When stat1 is LOW
		else if( stat0 )  				
		{
			usb_fault = 0x2;
			LOG("USB 1 Over-Current is detected. usb_fault : 0x%x \n", usb_fault);
			fault_status();
		}
		// Both stat0 and stat1 are LOW
		else 					
		{
			usb_fault = 0x3;
			LOG("USB 0 & 1 Over-Current is detected. usb_fault : 0x%x \n", usb_fault);
			fault_status();
		}

	}while(1);

	LOG("\nUSB Fault Exit....");
	return 0;
}

static int get_usb_fault()
{
	return usb_fault;
}

static const struct file_operations usb_fops = {
	.owner = THIS_MODULE,
	.open = usbfault_open,
	.read = usbfault_read_blocked,
	.write = usbfault_write,
	.ioctl = usbfault_ioctl,
	.release = usbfault_close,
};

static int __init usbfault_init(void) 
{	
	int major;
	int ret;

	chardev = MKDEV(USB_FAULT_MAJOR, 0);

	// register char device
	ret = register_chrdev_region(chardev, 1, MODULE_NAME);

	if( ret )
	{
		LOG("Error in alloc_chrdev_region %d \n", ret);
	}
	
	chdev = cdev_alloc();
	if( !chdev )
	{
		ret = -ENOMEM;
		return ret;
	}
	
	major  			= MAJOR(chardev);
	chdev->owner 	= THIS_MODULE;
	chdev->ops 		= &usb_fops;

	ret = cdev_add(chdev, chardev, 1);
	if( ret)
	{
		LOG("Error %d adding dev", ret);
	}
	// chr drv end

	printk("registered usb fault driver \n");

	
	// Init both USB 0 and USB 1
	Set_USB0_OCD_IN();
	Set_USB1_OCD_IN();

	fault_status();

    return ret;
}

static int usbfault_open(struct inode *inode, struct file *file)
{
	int ret;

	if( usbfault_running == 0 )
	{
		// Create a kernel thread
		usb_fault_tsk = kthread_create(usb_fault_run, NULL, "usb_faultd");

		if (IS_ERR(usb_fault_tsk)) {
			ret = PTR_ERR(usb_fault_tsk);    
			usb_fault_tsk = NULL;
			return ret;
		}

		usb_fault_tsk->flags |= PF_NOFREEZE;
		wake_up_process(usb_fault_tsk);  

		usbfault_running = 1;
	}
	else
		return -EIO;

	return 0;
}

static int usbfault_close(struct inode *inode, struct file *file)
{
	if( usbfault_running )
	{
		kthread_stop(usb_fault_tsk);
		usbfault_running = 0;
	}

	return 0;
}

static void __exit usbfault_exit(void)
{
	cdev_del(chdev);
	unregister_chrdev_region(chardev, 1);
	printk("unregistered usb fault driver \n");
}

static int loc_read_blocked (void)
{
	return 0;	
}

/*
static int loc_read_unblocked (void)
{
	return 0;	
}
*/

static ssize_t usbfault_read_blocked (struct file *pFile, char *pUserBuf, size_t count, loff_t *pOffset)
{
	int result = 0;
	int buff = loc_read_blocked();

	if (buff < 0)
		return buff;
	result = copy_to_user((unsigned int*)pUserBuf, (unsigned int *)&buff, sizeof(unsigned int));

	return result;
}

static ssize_t usbfault_write (struct file *pFile, const char *pUserBuf, size_t count, loff_t *pOffset)
{
	return 0;
}

static int usbfault_ioctl (struct inode *pInode, struct file *pFile, unsigned int cmd, unsigned long arg)
{
	int result = 0;
	int buff = 0;
	switch(cmd)
	{
		case USBFAULT_BLOCKED_READ:
			buff = get_usb_fault();
			LOG("IOCTL : buff : 0x%x\n", buff);
			break;
		case USBFAULT_UNBLOCKED_READ:
			buff = get_usb_fault();
			LOG("IOCTL : buff : 0x%x\n", buff);
			break;
		case USBFAULT_READ_DISABLE:
			if( usbfault_running )
			{
				kthread_stop(usb_fault_tsk);
				usbfault_running = 0;
			}
			buff = 0;
			LOG("IOCTL : buff : 0x%x\n", buff);
			break;
		default:
			break;
	}
	if (buff < 0) // Error
		return buff;

	result = copy_to_user((unsigned int*)arg, (unsigned int*)&buff, sizeof(unsigned int));

	return result;
}

module_init(usbfault_init);
module_exit(usbfault_exit);

MODULE_LICENSE("SAMSUNG");
MODULE_AUTHOR("Sang-yeon Park");
MODULE_DESCRIPTION("Driver for usb fault");
