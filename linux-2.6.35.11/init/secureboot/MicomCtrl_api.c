#include <linux/termios.h>
#include <linux/unistd.h>
#include <linux/types.h>
#include <linux/stat.h>
#include <linux/fcntl.h>
#include <linux/string.h>

#define _QUOTE(x)   #x
#define QUOTE(x)    _QUOTE(x)
#define MICOM_DEV_NAME  QUOTE(MICOM_DEV)

struct termios newtio;

#define BAUDRATE B9600

extern long sys_read(unsigned int fd, char *buf, long count);
extern int printk(const char * fmt, ...);
extern void * memcpy(void *, const void *, unsigned int);
extern long sys_close(unsigned int fd);
extern long sys_open(const char *filename, int flags, int mode);
extern long sys_write(unsigned int fd, const char __attribute__((noderef, address_space(1))) *buf, long count);                              
extern void msleep(unsigned int msecs);  

void bzero(void * buf,int size)
{
	char * p=buf;
	int i;
	for(i=0;i<size;i++)
	{
		*p=0;
		p++;
	}
}

int SerialInit(char *dev)
{
	int fd;
	
	fd = sys_open(dev, O_RDWR | O_NOCTTY,0 );//MS PARK
	if (fd <0) 
	{
		printk("Error..............\n");
		return (-1);
	}

	return fd;
	
	bzero((void *)&newtio, sizeof(newtio));

	/* 9600 BPS, Data 8 bit, 1 Stop Bit, NO flow control, No parity. */
	newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = OPOST;
	
	/* set input mode (non-canonical, no echo,...) */
	newtio.c_lflag = 0;

	newtio.c_cc[VTIME]    = 0;   /* 문자 사이의 timer를 disable */
	newtio.c_cc[VMIN]     = 1;   /* 최소 1 문자 받을 때까진 blocking */

	return fd;
}

int WriteSerialData(int fd, unsigned char *value, unsigned char num)
{
	int res;
	
	res = sys_write(fd, value, num);
	
	if (!res)
	{
		printk("Write Error!!\n");
	}
#ifdef POCKET_TEST
	if(!res)
	{
		printk("[!@#]MI_Err, ------------, -------\r\n");
    }
#endif // #ifdef POCKET_TEST
	return res;
}

int SerialClose(int fd)
{
	int retVar = 0;

	if(sys_close(fd))
    {
        retVar = 1;
	}
	return retVar;
}

int MicomCtrl(unsigned char ctrl)
{
	int fd;
	char serial_target[20];
	unsigned char databuff[9];

	memset(databuff, 0, 9);
	bzero(serial_target, 20);

	strcpy(serial_target, MICOM_DEV_NAME);
	fd = SerialInit(serial_target);

	databuff[0] = 0xff;
	databuff[1] = 0xff;
	databuff[2] = ctrl;
	databuff[8] += databuff[2];

	WriteSerialData(fd, databuff, 9);
	msleep(10);
	SerialClose(fd);

	return 0;
}

