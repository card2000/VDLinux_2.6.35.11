/* incluce/linux/sdp_dmadev.h
 * 
 *
 * Copyright (C) 2011 Samsung Electronics Co. Ltd.
 * Dongseok lee <drain.lee@samsung.com>
 *
 *
 * 
 */

 /*
 * 20110621	created by drain.lee
 */

#include <linux/types.h>
#include <linux/ioctl.h>


#ifndef _LINUX_SDP_DMADEV_H
#define _LINUX_SDP_DMADEV_H

#define SDP_DMADEV_IOC_MAGIC		0xA0

enum sdp_dmadev_cache_ctrl
{
	src,
};

enum pl330_srccachectrl {
	SCCTRL0 = 0, /* Noncacheable and nonbufferable */
	SCCTRL1, /* Bufferable only */
	SCCTRL2, /* Cacheable, but do not allocate */
	SCCTRL3, /* Cacheable and bufferable, but do not allocate */
	SINVALID1,
	SINVALID2,
	SCCTRL6, /* Cacheable write-through, allocate on reads only */
	SCCTRL7, /* Cacheable write-back, allocate on reads only */
};

enum pl330_dstcachectrl {
	DCCTRL0 = 0, /* Noncacheable and nonbufferable */
	DCCTRL1, /* Bufferable only */
	DCCTRL2, /* Cacheable, but do not allocate */
	DCCTRL3, /* Cacheable and bufferable, but do not allocate */
	DINVALID1 = 8,
	DINVALID2,
	DCCTRL6, /* Cacheable write-through, allocate on writes only */
	DCCTRL7, /* Cacheable write-back, allocate on writes only */
};

struct sdp_dmadev_ioctl_args {
	unsigned long src_addr;
	unsigned long dst_addr;
	size_t len;
};

/* for dma memcpy */
#define SDP_DMADEV_IOC_MEMCPY	_IOWR(SDP_DMADEV_IOC_MAGIC, 1, struct sdp_dmadev_ioctl_args)
#define SDP_DMADEV_IOC_MEMCPY1	_IOWR(SDP_DMADEV_IOC_MAGIC, 2, struct sdp_dmadev_ioctl_args)
#define SDP_DMADEV_IOC_MEMCPY2	_IOWR(SDP_DMADEV_IOC_MAGIC, 3, struct sdp_dmadev_ioctl_args)

#endif/*_LINUX_SDP_DMADEV_H*/