/* driver/dma/sdp_dmadev.c
 *
 * Copyright (C) 2011 Samsung Electronics Co. Ltd.
 * Dongseok lee <drain.lee@samsung.com>
 *
 */


/*
 * 20110617	created by drain.lee
 */


#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/cacheflush.h>

#include <linux/dmaengine.h>

#include <linux/sdp_dmadev.h>

extern struct dma_async_tx_descriptor *
sdp_dma330_prep_dma_memcpy_sg(struct dma_chan *chan, 
		struct scatterlist *dstsgl, struct scatterlist *srcsgl,
		size_t sg_len, unsigned long flags);

extern struct dma_async_tx_descriptor *
sdp_dma330_cache_ctrl(struct dma_chan *chan, 
	struct dma_async_tx_descriptor *tx, u32 dst_cache, u32 src_cache);
	

#define SDP_DMADEV_MAJOR	MAJOR(MKDEV(251, 0))
#define GET_PAGE_NUM(addr, len)		((PAGE_ALIGN(addr + len) -(addr & PAGE_MASK)) >> PAGE_SHIFT)
#define SDP_DMADEV_NAME		"sdp-dmadev"

//#define SDP_MUST_DMACPY


struct sdp_dmadev_data {
	struct device *dev;
};

/* create at open */
struct sdp_dmadev_chan {
	/* for dmaengine */
	int numofchans;
	struct dma_chan *chan;
	struct dma_chan *chan2;
	struct sdp_dmadev_data *sdpdmadev;
};

static struct sdp_dmadev_data *gsdpdmadev;


/* data util functions */
inline static struct sdp_dmadev_data *
to_sdpdmadev(struct sdp_dmadev_chan *chan)
{
	return container_of(chan, struct sdp_dmadev_chan, sdpdmadev);
}

inline static struct sdp_dmadev_chan *
to_sdpdmadevchan(struct dma_chan *chan)
{
	return container_of(chan, struct sdp_dmadev_chan, chan);
}


/* local functions*/
static void _page_dump(struct page *page)
{
	int i;
	char *start = (char*)page_address(page);
	pr_info("page dump 0x%08x", start);
	//print_hex_dump(KERN_INFO, "", DUMP_PREFIX_NONE, 16, 1, start, PAGE_SIZE, 0);
}


/* speed test */
static struct timeval start, finish;
static inline void _start_timing(void)
{
	do_gettimeofday(&start);
}

static inline void _stop_timing(void)
{
	do_gettimeofday(&finish);
}

static unsigned long _calc_speed(unsigned long bytes)
{
	unsigned long us, k, speed;

	us = (finish.tv_sec - start.tv_sec) * 1000000 +
	     (finish.tv_usec - start.tv_usec);
	k = bytes / 1024;

	if(us == 0) return 0;
	speed = (k * 1000000) / us;
	return speed;
}




static int 
_sdp_dmadev_memcpy(
	struct dma_chan *chan,
	unsigned long src_uaddr, unsigned long dest_uaddr, size_t len)
{
	int i;
	int ret, tmp;
	//const unsigned long end = (src_uaddr + len + PAGE_SIZE - 1) >> PAGE_SHIFT;
	//const unsigned long start = src_uaddr >> PAGE_SHIFT;
	const int src_nr_pages = GET_PAGE_NUM(src_uaddr, len);
	const int src_offset = src_uaddr & ~PAGE_MASK;
	struct dma_pinned_list *dest_pinned_list;
	struct iovec dest_iov = {
		.iov_base = dest_uaddr,
		.iov_len = len,
	};
	dma_cookie_t dma_cookie;
	struct page **src_pages = NULL;
	int copybytes = 0;

	src_pages = kzalloc(sizeof(*src_pages) * src_nr_pages, GFP_KERNEL);
	if(!src_pages)
		return -ENOMEM;

	dev_dbg(gsdpdmadev->dev, "src_pages(%p), src_nr_pages(%d)", src_pages, src_nr_pages);

	/* get src pages pinned */
	ret = get_user_pages_fast(src_uaddr, src_nr_pages, 0, src_pages);
	if( unlikely(ret < src_nr_pages) )
	{
		dev_err(gsdpdmadev->dev, "get_user_pages_fast() return with error(%d)", ret);
		goto err_2;
	}
		
	dest_pinned_list = dma_pin_iovec_pages(&dest_iov, len);
	if( unlikely(!dest_pinned_list) )
	{
		dev_err(gsdpdmadev->dev, "dma_pin_iovec_pages() return with error(%d)", ret);
		goto err_1;
	}

	for(i = 0; i < src_nr_pages; i++)
	{
		dev_dbg(gsdpdmadev->dev, "page%2d kernel_va_src=0x%lx\n", i, (unsigned long)page_address(src_pages[i]) );
	}

	for(i = 0; i < dest_pinned_list->page_list[0].nr_pages; i++)
	{
		dev_dbg(gsdpdmadev->dev, "page%2d kernel_va_dst=0x%lx\n", i, (unsigned long)page_address(dest_pinned_list->page_list[0].pages[i]) );
	}

	/* step1 : first page copy */
	tmp = min_t(int, PAGE_SIZE-src_offset, len);
	dma_cookie = dma_memcpy_pg_to_iovec(chan,
		&dest_iov,
		dest_pinned_list,
		*src_pages,
		src_offset,
		tmp );
	copybytes += tmp;
	if(IS_ERR(dma_cookie))
	{
		ret = dma_cookie;
		goto err_0;
	}
//	dma_async_issue_pending(chan);
//	while(dma_async_is_tx_complete(chan, dma_cookie, NULL, NULL) == DMA_IN_PROGRESS );
	/* step2 : 2 ~ N-1st page copy */
	for(i = 1; i < src_nr_pages-1; i++)
	{
		dma_cookie = dma_memcpy_pg_to_iovec(chan,
			&dest_iov,
			dest_pinned_list,
			*(src_pages + i),
			0,
			PAGE_SIZE);
		copybytes += PAGE_SIZE;
		if(IS_ERR(dma_cookie))
		{
			ret = dma_cookie;
			goto err_0;
		}
//		dma_async_issue_pending(chan);
//		while(dma_async_is_tx_complete(chan, dma_cookie, NULL, NULL) == DMA_IN_PROGRESS );
	}
	/* step3 : Nst page copy */
	if(src_nr_pages >= 2)
	{
		dma_cookie = dma_memcpy_pg_to_iovec(chan,
			&dest_iov,
			dest_pinned_list,
			*(src_pages + (src_nr_pages - 1)),
			0,
			len-copybytes );
		copybytes += len-copybytes;
		if(IS_ERR(dma_cookie))
		{
			ret = dma_cookie;
			goto err_0;
		}
	}
	dma_async_issue_pending(chan);
	while(dma_async_is_tx_complete(chan, dma_cookie, NULL, NULL) == DMA_IN_PROGRESS );
	
	if(copybytes != len)
		ret = -EIO;
	else
		ret = copybytes;

err_0:
	/* unpin dest pages */
	dma_unpin_iovec_pages(dest_pinned_list);
	
err_1:/* ummap src pages*/
	for (i=0; i < src_nr_pages; i++)
		page_cache_release(*(src_pages+ i) );

err_2:/* free src pages */
	kfree(src_pages);

	return ret;
}





static int _sdp_dma330_map(struct dma_chan *chan, 
	struct scatterlist *dst_sg,
	struct page *dst_page,
	struct page *dst_offset,
	struct scatterlist *src_sg,
	struct page *src_page,
	struct page *src_offset, int len)
{
	if(unlikely(!dst_sg || !src_sg || !dst_page || !src_page))
	{
		dev_err(gsdpdmadev->dev, "_sdp_dma330_map() NULL pointer dereference.\n");
		return -EINVAL;
	}
	
	sg_dma_address(dst_sg) = dma_map_page(&chan->dev->device,
		dst_page, dst_offset, len, DMA_FROM_DEVICE);
	sg_dma_address(src_sg) = dma_map_page(&chan->dev->device,
		src_page, src_offset, len,	DMA_TO_DEVICE);
	sg_dma_len(dst_sg) = len;
	sg_dma_len(src_sg) = len;

	dev_dbg(gsdpdmadev->dev, "dmamap addr src 0x%08x, dst 0x%08x, len %x(%d)\n",
		sg_dma_address(src_sg), sg_dma_address(dst_sg), len, len);
}

static inline void 
sdp_hwmem_flush_all(void)
{
	unsigned long flag;

	raw_local_irq_save (flag);

	__cpuc_flush_kern_all ();

#if defined(CONFIG_CACHE_L2X0)
	l2x0_flush_all(); 
#endif
	raw_local_irq_restore (flag);
}
static inline void 
sdp_hwmem_inv_range(const void* vir_addr, const unsigned long phy_addr, const size_t size)
{
	dmac_map_area (vir_addr, size, DMA_FROM_DEVICE);
	outer_inv_range(phy_addr, (unsigned long)(phy_addr+size));
}
static inline void 
sdp_hwmem_clean_range(const void* vir_addr, const unsigned long phy_addr, const size_t size)
{
	dmac_map_area (vir_addr, size, DMA_TO_DEVICE);
	outer_clean_range(phy_addr, (unsigned long)(phy_addr+size));
}

static inline void 
sdp_hwmem_flush_range(const void* vir_addr, const unsigned long phy_addr, const size_t size)
{
	dmac_flush_range(vir_addr,(const void *)(vir_addr+size));
	outer_flush_range(phy_addr, (unsigned long)(phy_addr+size));
}


static inline dma_addr_t _sdp_dmadev_vir_to_phys(u32 user_vaddr)
{
	unsigned long flags;

	u32 ttb;
	u32 user_paddr;
	u32 *ppage_1st, *ppage_2nd;


  /*
	 Translation Table Base 0 [31:14] | SBZ [13:5] | RGN[4:3] | IMP[2] | S[1] | C[0] 
  */
	local_irq_save(flags);

	asm volatile ("mrc p15, 0, %0, c2, c0, 0 \n" : "=r"(ttb));	
	ttb &= 0xFFFFC000;
	ppage_1st = phys_to_virt(ttb);

 /* address of 1st level descriptor */
	ppage_1st += (user_vaddr >> 20);
 /* 1st level descriptor */

 /* address of 2nd level descriptor */
	ppage_2nd = phys_to_virt(*ppage_1st);
	ppage_2nd = (u32)ppage_2nd & 0xFFFFFC00;
	ppage_2nd += (user_vaddr >> 12) & 0xFF; 
 /* 2nd level descriptor */
	user_paddr = ((u32)*ppage_2nd & 0xFFFFF000) | (user_vaddr & 0xFFF);

	local_irq_restore(flags);
	return user_paddr;
}



int 
_sdp_dmadev_memcpy_cnt(
	struct dma_chan *chan,
	unsigned long src_uaddr, unsigned long dst_uaddr, size_t len)
{
	int ret, i, copy;
	const int src_nr_pages = GET_PAGE_NUM(src_uaddr, len);
	const int dst_nr_pages = GET_PAGE_NUM(dst_uaddr, len);
	const int src_offset = src_uaddr & ~PAGE_MASK;
	const int dst_offset = dst_uaddr & ~PAGE_MASK;
	struct page **src_pages = NULL, **dst_pages = NULL;
	struct dma_pinned_list *dst_pinned_list;
	struct iovec dst_iov = {
		.iov_base = dst_uaddr,
		.iov_len = len,
	};
	dma_cookie_t dma_cookie;
	struct scatterlist *cursrc, *curdst;
	struct dma_async_tx_descriptor *tx = NULL;
	
	int copybytes = 0;
	_start_timing();

	/* alloc src dst Pages */
	src_pages = kzalloc(sizeof(*src_pages) * src_nr_pages, GFP_KERNEL);
	if(!src_pages)
		return -ENOMEM;

	dst_pages = kzalloc(sizeof(*dst_pages) * dst_nr_pages, GFP_KERNEL);
	if(!dst_pages)
	{
		kfree(src_pages);
		return -ENOMEM;
	}



	/* get user pages */
	dev_dbg(gsdpdmadev->dev, "src_pages(%p), src_nr_pages(%d)", src_pages, src_nr_pages);
	/* get src pages pinned */
	ret = get_user_pages_fast(src_uaddr, src_nr_pages, 0, src_pages);
	if( unlikely(ret < src_nr_pages) )
	{
		dev_err(gsdpdmadev->dev, "get_user_pages_fast() return with error(%d)", ret);
		goto err_3;
	}
	
	dev_dbg(gsdpdmadev->dev, "dst_pages(%p), dst_nr_pages(%d)", dst_pages, dst_nr_pages);
	/* get src pages pinned */
	ret = get_user_pages_fast(dst_uaddr, dst_nr_pages, 0, dst_pages);
	if( unlikely(ret < dst_nr_pages) )
	{
		dev_err(gsdpdmadev->dev, "get_user_pages_fast() return with error(%d)", ret);
		goto err_2;
	}
		
	
#if 0
	for(i = 0; i < src_nr_pages; i++)
	{
		dev_dbg(gsdpdmadev->dev, "page%2d kernel_va_src=0x%lx\n", i, (unsigned long)page_address(src_pages[i]) );
	}

	for(i = 0; i < dst_pinned_list->page_list[0].nr_pages; i++)
	{
		dev_dbg(gsdpdmadev->dev, "page%2d kernel_va_dst=0x%lx\n", i, (unsigned long)page_address(dst_pinned_list->page_list[0].pages[i]) );
	}
#endif


	struct sg_table sgtsrc, sgtdst;
	const int sg_nents = (src_offset == dst_offset ? src_nr_pages : (src_nr_pages*2)+1 );
	ret = sg_alloc_table(&sgtsrc, sg_nents, GFP_KERNEL);
	if(ret<0)
	{
		goto err_2;
	}
	ret = sg_alloc_table(&sgtdst, sg_nents, GFP_KERNEL);
	if(ret<0)
	{
		sg_free_table(&sgtsrc);
		goto err_2;
	}

	cursrc = sgtsrc.sgl;
	curdst = sgtdst.sgl;
	int curpagebyte = 0;
	
	/* step1 : src first page copy */
	curpagebyte = min_t(int, PAGE_SIZE-src_offset, len);
	copy = min_t(int, curpagebyte, PAGE_SIZE-dst_offset);

	_sdp_dma330_map(chan, curdst, dst_pages[0], dst_offset,
		cursrc, src_pages[0], src_offset, copy);

	copybytes += copy;

	if(copy < curpagebyte)
	{
		cursrc = sg_next(cursrc);
		curdst = sg_next(curdst);

		_sdp_dma330_map(chan, curdst,
			dst_pages[(copybytes+dst_offset)>>PAGE_SHIFT],
			((copybytes+dst_offset)&~PAGE_MASK),
			cursrc,
			src_pages[(copybytes+src_offset)>>PAGE_SHIFT],
			((copybytes+src_offset)&~PAGE_MASK),
			curpagebyte-copy);

		copybytes += curpagebyte-copy;
	}


	/* step2 : 2 ~ N-1st page copy */
	for(i = 1; i < src_nr_pages-1; i++)
	{
		curpagebyte = PAGE_SIZE;
		copy = min_t(int, curpagebyte, PAGE_SIZE-((copybytes+dst_offset)&~PAGE_MASK));
		cursrc = sg_next(cursrc);
		curdst = sg_next(curdst);

		_sdp_dma330_map(chan, curdst,
			dst_pages[(copybytes+dst_offset)>>PAGE_SHIFT],
			((copybytes+dst_offset)&~PAGE_MASK),
			cursrc,
			src_pages[(copybytes+src_offset)>>PAGE_SHIFT],
			((copybytes+src_offset)&~PAGE_MASK),
			copy);

		copybytes += copy;

		if(copy < curpagebyte)
		{
			cursrc = sg_next(cursrc);
			curdst = sg_next(curdst);

			_sdp_dma330_map(chan, curdst,
				dst_pages[(copybytes+dst_offset)>>PAGE_SHIFT],
				((copybytes+dst_offset)&~PAGE_MASK),
				cursrc,
				src_pages[(copybytes+src_offset)>>PAGE_SHIFT],
				((copybytes+src_offset)&~PAGE_MASK),
				curpagebyte-copy);

			copybytes += curpagebyte-copy;
		}

	}


	/* step3 : Nst page copy */
	if(src_nr_pages >= 2)
	{
		curpagebyte = len-copybytes;
		copy = min_t(int, curpagebyte, PAGE_SIZE-((copybytes+dst_offset)&~PAGE_MASK));
		cursrc = sg_next(cursrc);
		curdst = sg_next(curdst);

		_sdp_dma330_map(chan, curdst,
			dst_pages[(copybytes+dst_offset)>>PAGE_SHIFT],
			((copybytes+dst_offset)&~PAGE_MASK),
			cursrc,
			src_pages[(copybytes+src_offset)>>PAGE_SHIFT],
			((copybytes+src_offset)&~PAGE_MASK),
			copy);

		copybytes += copy;

		if(copy < curpagebyte)
		{
			cursrc = sg_next(cursrc);
			curdst = sg_next(curdst);

			_sdp_dma330_map(chan, curdst,
				dst_pages[(copybytes+dst_offset)>>PAGE_SHIFT],
				((copybytes+dst_offset)&~PAGE_MASK),
				cursrc,
				src_pages[(copybytes+src_offset)>>PAGE_SHIFT],
				((copybytes+src_offset)&~PAGE_MASK),
				curpagebyte-copy);

			copybytes += curpagebyte-copy;
		}
	}
	
	sg_mark_end(cursrc);
	sg_mark_end(curdst);


	_stop_timing();
	dev_info(gsdpdmadev->dev, "dma speed is %6dMB/s\n", _calc_speed(len)>>10);
	

	_start_timing();

	tx = sdp_dma330_prep_dma_memcpy_sg(chan, sgtdst.sgl, sgtsrc.sgl, sg_nents,
		DMA_CTRL_ACK|DMA_PREP_INTERRUPT|DMA_COMPL_SKIP_SRC_UNMAP|DMA_COMPL_SKIP_DEST_UNMAP);
	if(!tx)
	{
		dev_err(gsdpdmadev->dev, "prep error with src_addr=0x%x "
					"dst_addr=0x%x len=0x%x\n",
					src_uaddr, dst_uaddr, len);
		ret = -EIO;
		goto unmap;
	}

	dma_cookie = tx->tx_submit(tx);

	dma_async_issue_pending(chan);

	enum dma_status status = dma_sync_wait(chan, dma_cookie);

	if(status == DMA_ERROR)
	{
		dev_err(gsdpdmadev->dev, "dmaengine return error\n",
					src_uaddr, dst_uaddr, len);
		ret = -EIO;
	}
	
	while(dma_async_is_tx_complete(chan, dma_cookie, NULL, NULL) == DMA_IN_PROGRESS );

	_stop_timing();
	dev_info(gsdpdmadev->dev, "dma speed is %6dMB/s\n", _calc_speed(len)>>10);



unmap:
	for(cursrc = sgtsrc.sgl; cursrc; cursrc = sg_next(cursrc))
	{
		dma_unmap_page(&chan->dev->device, sg_dma_address(cursrc),
			sg_dma_len(cursrc), DMA_TO_DEVICE);
	}

	for(curdst = sgtsrc.sgl; curdst; curdst = sg_next(curdst))
	{
		dma_unmap_page(&chan->dev->device, sg_dma_address(curdst),
			sg_dma_len(curdst), DMA_FROM_DEVICE);
	}


err_0:/* ummap dst pages*/
	for (i=0; i < dst_nr_pages; i++)
	{
		/* dst page is dirty!! */
		set_page_dirty_lock(dst_pages[i]);
		page_cache_release(dst_pages[i]);
	}
	
err_1:/* ummap src pages*/
	for (i=0; i < src_nr_pages; i++)
		page_cache_release(src_pages[i]);

err_2:/* free dst pages */
	kfree(dst_pages);

err_3:/* free src pages */
	kfree(src_pages);
	
	return ret;
}


int 
_sdp_dmadev_memcpy_cnt_ttb(
	struct dma_chan *chan, struct dma_chan *chan2,
	const unsigned long src_uaddr, const unsigned long dst_uaddr, size_t len)
{
	int ret, i, copy;
	const int src_nr_pages = GET_PAGE_NUM(src_uaddr, len);
	const int dst_nr_pages = GET_PAGE_NUM(dst_uaddr, len);
	const int src_offset = src_uaddr & ~PAGE_MASK;
	const int dst_offset = dst_uaddr & ~PAGE_MASK;

	struct sdp_dmadev_chan *pdmadev = to_sdpdmadevchan(chan);

	dma_cookie_t dma_cookie;
	struct scatterlist *cursrc, *curdst;
	struct dma_async_tx_descriptor *tx = NULL;
	
	int copybytes = 0;
	_start_timing();

	dev_dbg(gsdpdmadev->dev, "src_nr_pages(%d)", src_nr_pages);
	dev_dbg(gsdpdmadev->dev, "dst_nr_pages(%d)", dst_nr_pages);

	struct sg_table sgtsrc, sgtdst;
	const int sg_nents = (src_offset == dst_offset ? src_nr_pages : (src_nr_pages*2)+1 );
	ret = sg_alloc_table(&sgtsrc, sg_nents, GFP_KERNEL);
	if(ret<0)
	{
		goto err_2;
	}
	ret = sg_alloc_table(&sgtdst, sg_nents, GFP_KERNEL);
	if(ret<0)
	{
		sg_free_table(&sgtsrc);
		goto err_2;
	}

	cursrc = sgtsrc.sgl;
	curdst = sgtdst.sgl;
	int curpagebyte = 0;
	
	/* step1 : src first page copy */
	curpagebyte = min_t(int, PAGE_SIZE-src_offset, len);
	copy = min_t(int, curpagebyte, PAGE_SIZE-dst_offset);

	sg_dma_address(cursrc) = _sdp_dmadev_vir_to_phys(src_uaddr);
	sg_dma_address(curdst) = _sdp_dmadev_vir_to_phys(dst_uaddr);
	sg_dma_len(cursrc) = copy;
	sg_dma_len(curdst) = copy;

	copybytes += copy;

	if(copy < curpagebyte)
	{
		cursrc = sg_next(cursrc);
		curdst = sg_next(curdst);

		sg_dma_address(cursrc) = _sdp_dmadev_vir_to_phys(src_uaddr + copybytes);
		sg_dma_address(curdst) = _sdp_dmadev_vir_to_phys(dst_uaddr + copybytes);
		sg_dma_len(cursrc) = curpagebyte-copy;
		sg_dma_len(curdst) = curpagebyte-copy;

		copybytes += curpagebyte-copy;
	}


	/* step2 : 2 ~ N-1st page copy */
	for(i = 1; i < src_nr_pages-1; i++)
	{
		curpagebyte = PAGE_SIZE;
		copy = min_t(int, curpagebyte, PAGE_SIZE-((copybytes+dst_offset)&~PAGE_MASK));
		cursrc = sg_next(cursrc);
		curdst = sg_next(curdst);

		sg_dma_address(cursrc) = _sdp_dmadev_vir_to_phys(src_uaddr + copybytes);
		sg_dma_address(curdst) = _sdp_dmadev_vir_to_phys(dst_uaddr + copybytes);
		sg_dma_len(cursrc) = copy;
		sg_dma_len(curdst) = copy;

		copybytes += copy;

		if(copy < curpagebyte)
		{
			cursrc = sg_next(cursrc);
			curdst = sg_next(curdst);

			sg_dma_address(cursrc) = _sdp_dmadev_vir_to_phys(src_uaddr + copybytes);
			sg_dma_address(curdst) = _sdp_dmadev_vir_to_phys(dst_uaddr + copybytes);
			sg_dma_len(cursrc) = curpagebyte-copy;
			sg_dma_len(curdst) = curpagebyte-copy;

			copybytes += curpagebyte-copy;
		}

	}


	/* step3 : Nst page copy */
	if(src_nr_pages >= 2)
	{
		curpagebyte = len-copybytes;
		copy = min_t(int, curpagebyte, PAGE_SIZE-((copybytes+dst_offset)&~PAGE_MASK));
		cursrc = sg_next(cursrc);
		curdst = sg_next(curdst);

		sg_dma_address(cursrc) = _sdp_dmadev_vir_to_phys(src_uaddr + copybytes);
		sg_dma_address(curdst) = _sdp_dmadev_vir_to_phys(dst_uaddr + copybytes);
		sg_dma_len(cursrc) = copy;
		sg_dma_len(curdst) = copy;

		copybytes += copy;

		if(copy < curpagebyte)
		{
			cursrc = sg_next(cursrc);
			curdst = sg_next(curdst);

			sg_dma_address(cursrc) = _sdp_dmadev_vir_to_phys(src_uaddr + copybytes);
			sg_dma_address(curdst) = _sdp_dmadev_vir_to_phys(dst_uaddr + copybytes);
			sg_dma_len(cursrc) = curpagebyte-copy;
			sg_dma_len(curdst) = curpagebyte-copy;

			copybytes += curpagebyte-copy;
		}
	}
	
	sg_mark_end(cursrc);
	sg_mark_end(curdst);


	_stop_timing();
	dev_info(gsdpdmadev->dev, "setup scatter speed is %6ldMB/s len 0x%x(%d)\n",
		_calc_speed(len)>>10, len, len);



	_start_timing();
#define CACHE_ALIGN_MASK	(0x3F)
	u32 curuaddr;
/* src cache flush */

#if 0
	curuaddr = src_uaddr;
	for(cursrc = sgtsrc.sgl; cursrc; cursrc = sg_next(cursrc))
	{
		sdp_hwmem_flush_range(curuaddr&~CACHE_ALIGN_MASK,
			sg_dma_address(cursrc)&~CACHE_ALIGN_MASK,
			(sg_dma_len(cursrc) + (curuaddr&CACHE_ALIGN_MASK) + CACHE_ALIGN_MASK)&~CACHE_ALIGN_MASK  );
		curuaddr += sg_dma_len(cursrc);
	}
#else
	sdp_hwmem_flush_all();
#endif

/* dst cache flush */
#if 0
	curuaddr = dst_uaddr;
	for(curdst = sgtdst.sgl; curdst; curdst = sg_next(curdst))
	{
		sdp_hwmem_flush_range(curuaddr&~CACHE_ALIGN_MASK,
			sg_dma_address(curdst)&~CACHE_ALIGN_MASK,
			(sg_dma_len(curdst) + (curuaddr&CACHE_ALIGN_MASK) + CACHE_ALIGN_MASK)&~CACHE_ALIGN_MASK  );
		curuaddr += sg_dma_len(curdst);
	}
#endif

	_stop_timing();
	dev_info(gsdpdmadev->dev, "Cache flush speed is %6ldMB/s len 0x%x(%d)\n",
		_calc_speed(len)>>10, len, len);
	








	_start_timing();
	cursrc = sgtsrc.sgl;
	i = 0;
	while(cursrc)
	{
		i++;
		cursrc = sg_next(cursrc);
	}
	int totalsg = i;

	if(i > 10 && chan2)
	{
		const div = totalsg/2;
		for(i = 0, cursrc = sgtsrc.sgl; cursrc && i<div; i++, cursrc = sg_next(cursrc));
		for(i = 0, curdst = sgtdst.sgl; curdst && i<div; i++, curdst = sg_next(curdst));
		
		
		tx = sdp_dma330_prep_dma_memcpy_sg(chan2, curdst, cursrc, totalsg-div,
			DMA_CTRL_ACK|DMA_PREP_INTERRUPT|DMA_COMPL_SKIP_SRC_UNMAP|DMA_COMPL_SKIP_DEST_UNMAP);

		if(!tx)
		{
			dev_err(gsdpdmadev->dev, "prep error with src_addr=0x%x "
						"dst_addr=0x%x len=0x%x\n",
						src_uaddr, dst_uaddr, len);
			ret = -EIO;
			goto unmap;
		}
		dma_cookie = tx->tx_submit(tx);
		dma_async_issue_pending(chan2);
		
		sg_mark_end(cursrc);
		sg_mark_end(curdst);
		tx = sdp_dma330_prep_dma_memcpy_sg(chan, sgtdst.sgl, sgtsrc.sgl, div,
			DMA_CTRL_ACK|DMA_PREP_INTERRUPT|DMA_COMPL_SKIP_SRC_UNMAP|DMA_COMPL_SKIP_DEST_UNMAP);
			

	}
	else
	{
		tx = sdp_dma330_prep_dma_memcpy_sg(chan, sgtdst.sgl, sgtsrc.sgl, i,
			DMA_CTRL_ACK|DMA_PREP_INTERRUPT|DMA_COMPL_SKIP_SRC_UNMAP|DMA_COMPL_SKIP_DEST_UNMAP);
	}
	if(!tx)
	{
		dev_err(gsdpdmadev->dev, "prep error with src_addr=0x%x "
					"dst_addr=0x%x len=0x%x\n",
					src_uaddr, dst_uaddr, len);
		ret = -EIO;
		goto unmap;
	}

	//tx = sdp_dma330_cache_ctrl(chan, tx, DCCTRL3, SCCTRL3);

	dma_cookie = tx->tx_submit(tx);

	dma_async_issue_pending(chan);

	enum dma_status status = dma_sync_wait(chan, dma_cookie);

	if(status == DMA_ERROR)
	{
		dev_err(gsdpdmadev->dev, "dmaengine return error\n",
					src_uaddr, dst_uaddr, len);
		ret = -EIO;
	}

	_stop_timing();
	dev_info(gsdpdmadev->dev, "excute dma330 speed is %6ldMB/s len 0x%x(%d)\n",
		_calc_speed(len)>>10, len, len);






	_start_timing();
/* cache invalidtate */
#if 0
	curuaddr = dst_uaddr;
	for(curdst = sgtdst.sgl; curdst; curdst = sg_next(curdst))
	{
		sdp_hwmem_inv_range(curuaddr&~CACHE_ALIGN_MASK,
			sg_dma_address(curdst)&~CACHE_ALIGN_MASK,
			(sg_dma_len(curdst) + (curuaddr&CACHE_ALIGN_MASK) + CACHE_ALIGN_MASK)&~CACHE_ALIGN_MASK  );
		curuaddr += sg_dma_len(curdst);
	}
#endif

	_stop_timing();
	dev_info(gsdpdmadev->dev, "cache invalidate speed is %6ldMB/s len 0x%x(%d)\n",
		_calc_speed(len)>>10, len, len);


unmap:

	


err_0:/* ummap dst pages*/
//	for (i=0; i < dst_nr_pages; i++)
//	{
		/* dst page is dirty!! */
//		set_page_dirty_lock(dst_pages[i]);
//		page_cache_release(dst_pages[i]);
//	}
	
err_1:/* ummap src pages*/
//	for (i=0; i < src_nr_pages; i++)
//		page_cache_release(src_pages[i]);

err_2:/* free dst pages */
//	kfree(dst_pages);

err_3:/* free src pages */
//	kfree(src_pages);
	
	return ret;
}

static long
sdp_dmadev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct sdp_dmadev_data *dmadev;
	struct sdp_dmadev_chan *sdpchan;
	struct dma_chan *chan;
	struct device *dev;

	struct sdp_dmadev_ioctl_args *ioctlarg;
	dma_cookie_t dma_cookie;
	
	int err;
	int tmp;
	

	if(_IOC_TYPE(cmd) != SDP_DMADEV_IOC_MAGIC)
		return -ENOTTY;

	/* Check access direction once here; don't repeat below.
	 * IOC_DIR is from the user perspective, while access_ok is
	 * from the kernel perspective; so they look reversed.
	 */
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE,	(void __user *)arg, _IOC_SIZE(cmd));
	if (err == 0 && _IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	if (err)
		return -EFAULT;
	/* guard against device removal before, or while,
	 * we issue this ioctl.
	 */
	sdpchan = filp->private_data;/*this is driver data in device */
	dmadev = to_sdpdmadev(sdpchan);
	chan = sdpchan->chan;
	
	//spin_lock_irq(&spidev->spi_lock);
	//spi = spi_dev_get(spidev->spi);
	//spin_unlock_irq(&spidev->spi_lock);
	if (unlikely(chan == NULL))
	{
#ifdef SDP_MUST_DMACPY
		return -ESHUTDOWN;
#else
		dev_dbg(gsdpdmadev->dev, "is not usable dmaengine channel. do software copy.\n");
#endif
	}

	/* use the buffer lock here for triple duty:
	 *  - prevent I/O (from us) so calling spi_setup() is safe;
	 *  - prevent concurrent SPI_IOC_WR_* from morphing
	 *    data fields while SPI_IOC_RD_* reads them;
	 *  - SPI_IOC_MESSAGE needs the buffer locked "normally".
	 */
	//mutex_lock(&spidev->buf_lock);

	switch (cmd) {
	case SDP_DMADEV_IOC_MEMCPY:
		tmp = _IOC_SIZE(cmd);
		ioctlarg = kzalloc(tmp, GFP_KERNEL);
		if( IS_ERR_OR_NULL(ioctlarg) )
		{
			err = -ENOMEM;
			break;
		}

		copy_from_user(ioctlarg, arg, tmp);
		dev_dbg(gsdpdmadev->dev, "SDP_DMADEV_IOC_MEMCPY srcdddr= 0x%lx, dstaddr= 0x%lx, len=0x%x%d\n",
			ioctlarg->src_addr, ioctlarg->dst_addr, ioctlarg->len, ioctlarg->len);

		err = is_dma_copy_aligned(gsdpdmadev, ioctlarg->src_addr, ioctlarg->dst_addr, ioctlarg->len);
		if(err)
		{
			dev_err(gsdpdmadev->dev, "address is not aligned.\n");
			return -EINVAL;
		}
//		_start_timing();
		err = _sdp_dmadev_memcpy_cnt_ttb(chan, NULL, ioctlarg->src_addr,
			ioctlarg->dst_addr, ioctlarg->len);
//		_stop_timing();
//		dev_info(gsdpdmadev->dev, "_sdp_dmadev_memcpy_cnt() speed is %6dKiB/s\n", _calc_speed(ioctlarg->len));
		
		kfree(ioctlarg);
		
		if(IS_ERR(err))
		{
			dev_dbg(gsdpdmadev->dev, "_sdp_dmadev_memcpy returned with error(%d)\n", err);
			break;
		}
		break;



	case SDP_DMADEV_IOC_MEMCPY1:
		tmp = _IOC_SIZE(cmd);
		ioctlarg = kzalloc(tmp, GFP_KERNEL);
		if( IS_ERR_OR_NULL(ioctlarg) )
		{
			err = -ENOMEM;
			break;
		}

		copy_from_user(ioctlarg, arg, tmp);
		dev_dbg(gsdpdmadev->dev, "SDP_DMADEV_IOC_MEMCPY srcdddr= 0x%lx, dstaddr= 0x%lx, len=0x%x%d\n",
			ioctlarg->src_addr, ioctlarg->dst_addr, ioctlarg->len, ioctlarg->len);

		err = is_dma_copy_aligned(gsdpdmadev, ioctlarg->src_addr, ioctlarg->dst_addr, ioctlarg->len);
		if(err)
		{
			dev_err(gsdpdmadev->dev, "address is not aligned.\n");
			return -EINVAL;
		}
//		_start_timing();
		err = _sdp_dmadev_memcpy_cnt_ttb(chan, sdpchan->chan2, ioctlarg->src_addr,
			ioctlarg->dst_addr, ioctlarg->len);
//		_stop_timing();
//		dev_info(gsdpdmadev->dev, "_sdp_dmadev_memcpy_cnt() speed is %6dKiB/s\n", _calc_speed(ioctlarg->len));
		
		kfree(ioctlarg);
		
		if(IS_ERR(err))
		{
			dev_dbg(gsdpdmadev->dev, "_sdp_dmadev_memcpy returned with error(%d)\n", err);
			break;
		}
		break;




	case SDP_DMADEV_IOC_MEMCPY2:
		tmp = _IOC_SIZE(cmd);
		ioctlarg = kzalloc(tmp, GFP_KERNEL);
		if( IS_ERR_OR_NULL(ioctlarg) )
		{
			err = -ENOMEM;
			break;
		}

		copy_from_user(ioctlarg, arg, tmp);
		dev_dbg(gsdpdmadev->dev, "SDP_DMADEV_IOC_MEMCPY2 srcdddr= 0x%lx, dstaddr= 0x%lx, len=0x%x%d\n",
			ioctlarg->src_addr, ioctlarg->dst_addr, ioctlarg->len, ioctlarg->len);

		err = is_dma_copy_aligned(gsdpdmadev, ioctlarg->src_addr, ioctlarg->dst_addr, ioctlarg->len);
		if(err)
		{
			dev_err(gsdpdmadev->dev, "address is not aligned.\n");
			return -EINVAL;
		}
//		_start_timing();
		err = _sdp_dmadev_memcpy_cnt(chan, ioctlarg->src_addr,
			ioctlarg->dst_addr, ioctlarg->len);
//		_stop_timing();
//		dev_info(gsdpdmadev->dev, "_sdp_dmadev_memcpy_cnt() speed is %6dKiB/s\n", _calc_speed(ioctlarg->len));
		
		kfree(ioctlarg);
		
		if(IS_ERR(err))
		{
			dev_dbg(gsdpdmadev->dev, "_sdp_dmadev_memcpy returned with error(%d)\n", err);
			break;
		}
		break;
	default:
		break;
	}

	return err;

#if 0
	/* test user space vitual address to page */
	int i, j;
	int res = 0;
	unsigned int count = cmd;
	unsigned long uaddr = arg;
	unsigned long end = (uaddr + count + PAGE_SIZE - 1) >> PAGE_SHIFT;
	unsigned long start = uaddr >> PAGE_SHIFT;
	const int nr_pages = end - start;
	struct task_struct *task = current;
	int maxpages = 256;//4k*256 = 1M
	struct page **pages;

	pr_info("cmd=%#x, arg=%#lx\n", cmd, arg);	
	pr_info("user addr=%#lx\n", uaddr);

	pages = kzalloc(sizeof(struct page) * maxpages, GFP_KERNEL);
	if(pages == NULL)
	{
		return -ENOMEM;
	}
	down_read(&task->mm->mmap_sem);
	res = get_user_pages(task, task->mm, uaddr, nr_pages, 1, 0, pages, NULL);
	up_read(&task->mm->mmap_sem);

	/* Errors and no page mapped should return here */
	if (res < nr_pages)
		goto out_unmap;

    for (i=0; i < nr_pages; i++) {
        /* FIXME: flush superflous for rw==READ,
         * probably wrong function for rw==WRITE
         */
		flush_dcache_page(pages[i]);
		
		pr_info("pageno=%d address=%#lx\n", i, (unsigned long)page_address(pages[i]));
    }

    for (i=0; i < nr_pages; i++) {
    	_page_dump(pages[i]);
    }

    //dma_map_page(struct device * dev, struct page * page, unsigned long offset, size_t size, enum dma_data_direction dir)

	//mdata->offset = uaddr & ~PAGE_MASK;
	//STbp->mapped_pages = pages;

	//return nr_pages;
	dma_cap_mask_t dma_cap;
	dma_cap_zero(dma_cap);
	dma_cap_set(DMA_MEMCPY, dma_cap);
	struct dma_chan *chan = dma_request_channel(dma_cap, NULL, NULL);
	struct dma_device *dma_dev = chan->device;
	dma_addr_t dmasrc = dma_map_page(dma_dev->dev, *pages, uaddr & ~PAGE_MASK, count, DMA_BIDIRECTIONAL);
	pr_info("dma_addr_t 0x%08x", dmasrc);
	dma_unmap_page(dma_dev->dev, dmasrc, count, DMA_BIDIRECTIONAL);
	dma_release_channel(chan);
	
	
 out_unmap:
	if (res > 0) {
		for (i=0; i < res; i++)
			page_cache_release(pages[i]);
		res = 0;
	}
	kfree(pages);
	return res;
#endif
}


static int sdp_dmadev_open(struct inode *inode, struct file *filp)
{
	struct sdp_dmadev_chan *sdpchan;
	struct dma_chan *chan;
	dma_cap_mask_t dma_cap;



	sdpchan = kzalloc(sizeof(*sdpchan), GFP_KERNEL);
	if( IS_ERR_OR_NULL(sdpchan) )
	{
		pr_debug(SDP_DMADEV_NAME ": do not allocate memory(%ld).\n", ENOMEM);
		return -ENOMEM;
	}


	dma_cap_zero(dma_cap);
	dma_cap_set(DMA_MEMCPY, dma_cap);
	chan = dma_request_channel(dma_cap, NULL, NULL);
	if( IS_ERR_OR_NULL(chan) )
	{
		pr_debug(SDP_DMADEV_NAME ": do not allocated channel(%ld).\n", PTR_ERR(chan));
#ifdef SDP_MUST_DMACPY
		kfree(sdpchan);
		return -EBUSY;
#endif
	}

	//pr_debug(SDP_DMADEV_NAME ": allocated dma channel (%s)\n", dma_chan_name(chan));
	sdpchan->sdpdmadev = gsdpdmadev;
	sdpchan->chan = chan;
	sdpchan->numofchans = 1;
	filp->private_data = sdpchan;
	
	/*one more req chan*/
	chan = dma_request_channel(dma_cap, NULL, NULL);
	if( !IS_ERR_OR_NULL(chan) )
	{
		sdpchan->chan2 = chan;
		sdpchan->numofchans = 2;
		pr_info(SDP_DMADEV_NAME ": 2nd channel able.\n");
	}

	
	
	return nonseekable_open(inode, filp);

}
static int sdp_dmadev_release(struct inode *inode, struct file *filp)
{
	struct sdp_dmadev_chan *sdpchan;

	sdpchan = (struct sdp_dmadev_chan*)filp->private_data;
	filp->private_data = 0;
	if(sdpchan)
	{
		if(sdpchan->chan)
			dma_release_channel(sdpchan->chan);
		if(sdpchan->numofchans > 1 && sdpchan->chan2)
		{
			dma_release_channel(sdpchan->chan2);
		}
		kzfree(sdpchan);
	}


	return 0;
}

static const struct file_operations sdp_dmadev_fops = {
	.owner =	THIS_MODULE,
	/* REVISIT switch to aio primitives, so that userspace
	 * gets more complete API coverage.  It'll simplify things
	 * too, except for the locking.
	 */
	//.write =	spidev_write,
	//.read =		spidev_read,
	.unlocked_ioctl = sdp_dmadev_ioctl,
	.open =		sdp_dmadev_open,
	.release =	sdp_dmadev_release,
};


static struct class *sdp_dmadev_class;

static int __init sdp_dmadev_init(void)
{
	int err = 0;
	struct device *dev;
	dev_t devt;
	struct sdp_dmadev_data *sdpdmadev;

	err = register_chrdev(SDP_DMADEV_MAJOR, SDP_DMADEV_NAME, &sdp_dmadev_fops);
	if(err < 0)
	{
		pr_err("register_chrdev error(%d)", err);
		return err;
	}

	sdp_dmadev_class = class_create(THIS_MODULE, SDP_DMADEV_NAME);
	if (IS_ERR(sdp_dmadev_class)) {
		pr_err("class_create error(%d)", err);
		goto err0;
	}
	
	devt = MKDEV(SDP_DMADEV_MAJOR, 0);
	dev = device_create(sdp_dmadev_class, NULL, devt, NULL/*drv data*/,
		"sdp_dmadev");

	if( IS_ERR(dev) )
	{
		pr_err("device_create error(%d)", PTR_ERR(dev));
		goto err1;
	}

	/* create driver data */
	sdpdmadev = kzalloc(sizeof(*sdpdmadev), GFP_KERNEL);
	if( IS_ERR_OR_NULL(sdpdmadev) )
	{
		err -ENOMEM;
		goto err2;
	}
	sdpdmadev->dev = dev;
	dev_set_drvdata(dev, sdpdmadev);
	gsdpdmadev = sdpdmadev;

	dev_info(dev, "init call done. dev node is /dev/sdp_dmadev");
	return 0;


err2:
	device_destroy(sdp_dmadev_class, devt);
err1:
	class_destroy(sdp_dmadev_class);
err0:/* chrdev unregister */
	unregister_chrdev(SDP_DMADEV_MAJOR, SDP_DMADEV_NAME);
	return err;
}
module_init(sdp_dmadev_init);


static void __exit sdp_dmadev_exit(void)
{
	if(gsdpdmadev)
	{
		if(sdp_dmadev_class)
		{
			device_destroy(sdp_dmadev_class, gsdpdmadev->dev->devt);
			class_destroy(sdp_dmadev_class);
		}
		kfree(gsdpdmadev);
		gsdpdmadev = NULL;
	}
	unregister_chrdev(SDP_DMADEV_MAJOR, SDP_DMADEV_NAME);
}
module_exit(sdp_dmadev_exit);

MODULE_AUTHOR("Dongseok lee <drain.lee@samsung.com>");
MODULE_DESCRIPTION("User level device for DMAEngine");
MODULE_LICENSE("GPL");
MODULE_ALIAS("dma:sdp-dmadev");

