/*
 *  linux/drivers/mmc/card/queue.c
 *
 *  Copyright (C) 2003 Russell King, All Rights Reserved.
 *  Copyright 2006-2007 Pierre Ossman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/blkdev.h>
#include <linux/freezer.h>
#include <linux/kthread.h>
#include <linux/scatterlist.h>

#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include "queue.h"


#ifdef CONFIG_NVT_CHIP
#include "../host/nt72558_mmc.h"

#ifndef KEEP_ORIGINAL_eMMC_READ
extern volatile struct mmc_queue *nvt_eMMC_mq;
extern volatile unsigned int nvt_eMMC_read_copied_length;
size_t sg_copy_from_buffer_for_nvt_eMMC(struct scatterlist *sgl, unsigned int nents,
			 void *buf, size_t buflen, size_t start_offset, size_t copy_length);
#endif // #ifndef KEEP_ORIGINAL_eMMC_READ

#ifdef eMMC_DMA_MEMCOPY_USE_INT_FOR_READ
extern volatile unsigned int nvt_eMMC_SDC_INT_DMA_INT_Counter;
#endif

#ifdef eMMC_DMA_MEMCOPY_USE_POLLING
extern volatile unsigned int Memory_Copy_DMA_Start_Address;
#endif


#ifdef eMMC_TEST_WRIE_WITH_READ_BACK_CHECK
char *eMMC_readback_check_buf;
volatile unsigned int eMMC_readback_check_dwSectorAddress;
EXPORT_SYMBOL(eMMC_readback_check_dwSectorAddress);
volatile unsigned int eMMC_readback_check_dwSectorCount;
EXPORT_SYMBOL(eMMC_readback_check_dwSectorCount);
volatile unsigned int eMMC_readback_check_dwSectorSize = 512;
EXPORT_SYMBOL(eMMC_readback_check_dwSectorSize);
extern unsigned int high_capacity;
volatile unsigned int dwRCA = 0;
EXPORT_SYMBOL(dwRCA);
#endif


#ifdef eMMC_REMOVE_DMA_MEMCOPY
char *eMMC_scatterlist_buf;
char *eMMC_scatterlist_buf_1;
struct scatterlist_dma {		
		dma_addr_t	dma_address;
		unsigned int	dma_length;
};
EXPORT_SYMBOL(eMMC_scatterlist_buf);
EXPORT_SYMBOL(eMMC_scatterlist_buf_1);
int Arrange_eMMC_DMA_Buffer(char* buf_1, char* buf, int DMA_buf_count);

volatile unsigned int eMMC_DMA_Buffer_Alignment_OK = 0;
EXPORT_SYMBOL(eMMC_DMA_Buffer_Alignment_OK);
void mmc_queue_bounce_pre_redo(struct mmc_queue *mq);
EXPORT_SYMBOL(mmc_queue_bounce_pre_redo);
#endif

#if defined(eMMC_DMA_MEMCOPY_USE_INT_FOR_READ) || defined(eMMC_TEST_WRIE_WITH_READ_BACK_CHECK)
#include <linux/delay.h>
#endif
#endif // #ifdef CONFIG_NVT_CHIP


#ifdef CONFIG_NVT_CHIP
#define MMC_QUEUE_BOUNCESZ	262144
#else
#define MMC_QUEUE_BOUNCESZ	65536
#endif

#define MMC_QUEUE_SUSPENDED	(1 << 0)

/*
 * Prepare a MMC request. This just filters out odd stuff.
 */
static int mmc_prep_request(struct request_queue *q, struct request *req)
{
	/*
	 * We only like normal block requests.
	 */
	if (!blk_fs_request(req)) {
		blk_dump_rq_flags(req, "MMC bad request");
		return BLKPREP_KILL;
	}

	req->cmd_flags |= REQ_DONTPREP;

	return BLKPREP_OK;
}

static int mmc_queue_thread(void *d)
{
	struct mmc_queue *mq = d;
	struct request_queue *q = mq->queue;

	current->flags |= PF_MEMALLOC;

	down(&mq->thread_sem);
	do {
		struct request *req = NULL;

		spin_lock_irq(q->queue_lock);
		set_current_state(TASK_INTERRUPTIBLE);
		if (!blk_queue_plugged(q))
			req = blk_fetch_request(q);
		mq->req = req;
		spin_unlock_irq(q->queue_lock);

		if (!req) {
			if (kthread_should_stop()) {
				set_current_state(TASK_RUNNING);
				break;
			}
			up(&mq->thread_sem);
			schedule();
			down(&mq->thread_sem);
			continue;
		}
		set_current_state(TASK_RUNNING);

		mq->issue_fn(mq, req);
	} while (1);
	up(&mq->thread_sem);

	return 0;
}

/*
 * Generic MMC request handler.  This is called for any queue on a
 * particular host.  When the host is not busy, we look for a request
 * on any queue on this host, and attempt to issue it.  This may
 * not be the queue we were asked to process.
 */
static void mmc_request(struct request_queue *q)
{
	struct mmc_queue *mq = q->queuedata;
	struct request *req;

	if (!mq) {
		while ((req = blk_fetch_request(q)) != NULL) {
			req->cmd_flags |= REQ_QUIET;
			__blk_end_request_all(req, -EIO);
		}
		return;
	}

	if (!mq->req)
		wake_up_process(mq->thread);
}

/**
 * mmc_init_queue - initialise a queue structure.
 * @mq: mmc queue
 * @card: mmc card to attach this queue
 * @lock: queue lock
 *
 * Initialise a MMC card request queue.
 */
int mmc_init_queue(struct mmc_queue *mq, struct mmc_card *card, spinlock_t *lock)
{
	struct mmc_host *host = card->host;
	u64 limit = BLK_BOUNCE_HIGH;
	int ret;

	if (mmc_dev(host)->dma_mask && *mmc_dev(host)->dma_mask)
		limit = *mmc_dev(host)->dma_mask;

	mq->card = card;
	mq->queue = blk_init_queue(mmc_request, lock);
	if (!mq->queue)
		return -ENOMEM;

	mq->queue->queuedata = mq;
	mq->req = NULL;

	blk_queue_prep_rq(mq->queue, mmc_prep_request);
	blk_queue_ordered(mq->queue, QUEUE_ORDERED_DRAIN, NULL);
	queue_flag_set_unlocked(QUEUE_FLAG_NONROT, mq->queue);

#ifdef CONFIG_MMC_BLOCK_BOUNCE
	if (host->max_hw_segs == 1) {
		unsigned int bouncesz;

		bouncesz = MMC_QUEUE_BOUNCESZ;

		if (bouncesz > host->max_req_size)
			bouncesz = host->max_req_size;
		if (bouncesz > host->max_seg_size)
			bouncesz = host->max_seg_size;
		if (bouncesz > (host->max_blk_count * 512))
			bouncesz = host->max_blk_count * 512;

		
		#ifndef CONFIG_NVT_CHIP
	
		if (bouncesz > 512) {
			mq->bounce_buf = kmalloc(bouncesz, GFP_KERNEL);
			if (!mq->bounce_buf) {
				printk(KERN_WARNING "%s: unable to "
					"allocate bounce buffer\n",
					mmc_card_name(card));
			}
		}

		#else // #ifndef CONFIG_NVT_CHIP

			#ifdef eMMC_DMA_MEMCOPY_USE_INT_FOR_READ

			if (bouncesz > 512) {
				mq->bounce_buf = kmalloc(bouncesz, GFP_KERNEL);
				if (!mq->bounce_buf) {
					printk(KERN_WARNING "%s: unable to "
						"allocate bounce buffer\n",
						mmc_card_name(card));
				}

				if ((unsigned int)(mq->bounce_buf) % eMMC_DMA_BUFFER_BOUND_SIZE_For_READ)
				{
					kfree(mq->bounce_buf);
					mq->bounce_buf = kmalloc(bouncesz + eMMC_DMA_BUFFER_BOUND_SIZE_For_READ, GFP_KERNEL);
					if (!mq->bounce_buf) {
					printk(KERN_WARNING "%s: unable to "
						"allocate bounce buffer ~ 1\n",
						mmc_card_name(card));
					}

					if ((unsigned int)(mq->bounce_buf) % eMMC_DMA_BUFFER_BOUND_SIZE_For_READ)
					{
						mq->bounce_buf = (char *)((unsigned int)(mq->bounce_buf)/eMMC_DMA_BUFFER_BOUND_SIZE_For_READ * eMMC_DMA_BUFFER_BOUND_SIZE_For_READ + eMMC_DMA_BUFFER_BOUND_SIZE_For_READ);
					}
				}

				printk(KERN_INFO "mq->bounce_buf = 0x%x ", (unsigned int)(mq->bounce_buf));
			}
			
			#else // #ifdef eMMC_DMA_MEMCOPY_USE_INT_FOR_READ

			if (bouncesz > 512) {
				mq->bounce_buf = kmalloc(bouncesz, GFP_KERNEL);
				if (!mq->bounce_buf) {
					printk(KERN_WARNING "%s: unable to "
						"allocate bounce buffer\n",
						mmc_card_name(card));
				}
			}

			#endif // #else // #ifdef eMMC_DMA_MEMCOPY_USE_INT_FOR_READ

		#endif // #else // #ifndef CONFIG_NVT_CHIP
		

		if (mq->bounce_buf) {
			blk_queue_bounce_limit(mq->queue, BLK_BOUNCE_ANY);
			blk_queue_max_hw_sectors(mq->queue, bouncesz / 512);
			blk_queue_max_segments(mq->queue, bouncesz / 512);
			blk_queue_max_segment_size(mq->queue, bouncesz);

			mq->sg = kmalloc(sizeof(struct scatterlist),
				GFP_KERNEL);
			if (!mq->sg) {
				ret = -ENOMEM;
				goto cleanup_queue;
			}
			sg_init_table(mq->sg, 1);

			mq->bounce_sg = kmalloc(sizeof(struct scatterlist) *
				bouncesz / 512, GFP_KERNEL);
			if (!mq->bounce_sg) {
				ret = -ENOMEM;
				goto cleanup_queue;
			}
			sg_init_table(mq->bounce_sg, bouncesz / 512);
		}
	}
#endif

	if (!mq->bounce_buf) {
		blk_queue_bounce_limit(mq->queue, limit);
		blk_queue_max_hw_sectors(mq->queue,
			min(host->max_blk_count, host->max_req_size / 512));
		blk_queue_max_segments(mq->queue, host->max_hw_segs);
		blk_queue_max_segment_size(mq->queue, host->max_seg_size);

		mq->sg = kmalloc(sizeof(struct scatterlist) *
			host->max_phys_segs, GFP_KERNEL);
		if (!mq->sg) {
			ret = -ENOMEM;
			goto cleanup_queue;
		}
		sg_init_table(mq->sg, host->max_phys_segs);
	}

	
	#ifdef CONFIG_NVT_CHIP
	#ifdef eMMC_TEST_WRIE_WITH_READ_BACK_CHECK
	
	eMMC_readback_check_buf = kmalloc(MMC_QUEUE_BOUNCESZ, GFP_KERNEL);
	if (!eMMC_readback_check_buf) {
		printk(KERN_WARNING "unable to "
			"allocate eMMC read back check buffer\n");
	}
		
	#endif	
	
	#ifdef eMMC_REMOVE_DMA_MEMCOPY

	eMMC_scatterlist_buf = kmalloc(sizeof(struct scatterlist_dma)*MMC_QUEUE_BOUNCESZ/4096, GFP_KERNEL); // minimum scatterlisr buffer size is 4KB.
	if (!eMMC_scatterlist_buf) {
		printk(KERN_WARNING "unable to "
			"allocate eMMC scatterlist buffer\n");
	}

	eMMC_scatterlist_buf_1 = kmalloc(sizeof(struct scatterlist_dma)*MMC_QUEUE_BOUNCESZ/4096, GFP_KERNEL); // each DMA buffer size is 4KB.
	if (!eMMC_scatterlist_buf_1) {
		printk(KERN_WARNING "unable to "
			"allocate eMMC scatterlist buffer 1\n");
	}
	
	#endif	
	#endif // #ifdef CONFIG_NVT_CHIP

	init_MUTEX(&mq->thread_sem);

	mq->thread = kthread_run(mmc_queue_thread, mq, "mmcqd");
	if (IS_ERR(mq->thread)) {
		ret = PTR_ERR(mq->thread);
		goto free_bounce_sg;
	}

	return 0;
 free_bounce_sg:
 	if (mq->bounce_sg)
 		kfree(mq->bounce_sg);
 	mq->bounce_sg = NULL;
 cleanup_queue:
 	if (mq->sg)
		kfree(mq->sg);
	mq->sg = NULL;
	if (mq->bounce_buf)
		kfree(mq->bounce_buf);
	mq->bounce_buf = NULL;
	blk_cleanup_queue(mq->queue);
	return ret;
}

void mmc_cleanup_queue(struct mmc_queue *mq)
{
	struct request_queue *q = mq->queue;
	unsigned long flags;

	/* Make sure the queue isn't suspended, as that will deadlock */
	mmc_queue_resume(mq);

	/* Then terminate our worker thread */
	kthread_stop(mq->thread);

	/* Empty the queue */
	spin_lock_irqsave(q->queue_lock, flags);
	q->queuedata = NULL;
	blk_start_queue(q);
	spin_unlock_irqrestore(q->queue_lock, flags);

 	if (mq->bounce_sg)
 		kfree(mq->bounce_sg);
 	mq->bounce_sg = NULL;

	kfree(mq->sg);
	mq->sg = NULL;

	if (mq->bounce_buf)
		kfree(mq->bounce_buf);
	mq->bounce_buf = NULL;

	mq->card = NULL;
}
EXPORT_SYMBOL(mmc_cleanup_queue);

/**
 * mmc_queue_suspend - suspend a MMC request queue
 * @mq: MMC queue to suspend
 *
 * Stop the block request queue, and wait for our thread to
 * complete any outstanding requests.  This ensures that we
 * won't suspend while a request is being processed.
 */
void mmc_queue_suspend(struct mmc_queue *mq)
{
	struct request_queue *q = mq->queue;
	unsigned long flags;

	if (!(mq->flags & MMC_QUEUE_SUSPENDED)) {
		mq->flags |= MMC_QUEUE_SUSPENDED;

		spin_lock_irqsave(q->queue_lock, flags);
		blk_stop_queue(q);
		spin_unlock_irqrestore(q->queue_lock, flags);

		down(&mq->thread_sem);
	}
}

/**
 * mmc_queue_resume - resume a previously suspended MMC request queue
 * @mq: MMC queue to resume
 */
void mmc_queue_resume(struct mmc_queue *mq)
{
	struct request_queue *q = mq->queue;
	unsigned long flags;

	if (mq->flags & MMC_QUEUE_SUSPENDED) {
		mq->flags &= ~MMC_QUEUE_SUSPENDED;

		up(&mq->thread_sem);

		spin_lock_irqsave(q->queue_lock, flags);
		blk_start_queue(q);
		spin_unlock_irqrestore(q->queue_lock, flags);
	}
}

/*
 * Prepare the sg list(s) to be handed of to the host driver
 */
unsigned int mmc_queue_map_sg(struct mmc_queue *mq)
{
	unsigned int sg_len;
	size_t buflen;
	struct scatterlist *sg;
	int i;

	if (!mq->bounce_buf)
		return blk_rq_map_sg(mq->queue, mq->req, mq->sg);

	BUG_ON(!mq->bounce_sg);

	sg_len = blk_rq_map_sg(mq->queue, mq->req, mq->bounce_sg);

	mq->bounce_sg_len = sg_len;

	buflen = 0;
	for_each_sg(mq->bounce_sg, sg, sg_len, i)
		buflen += sg->length;

	sg_init_one(mq->sg, mq->bounce_buf, buflen);

	return 1;
}

/*
 * If writing, bounce the data to the buffer before the request
 * is sent to the host driver
 */
void mmc_queue_bounce_pre(struct mmc_queue *mq)
{
	unsigned long flags;

	if (!mq->bounce_buf)
		return;

	if (rq_data_dir(mq->req) != WRITE)
		return;

			
	#ifdef eMMC_REMOVE_DMA_MEMCOPY
	#ifndef eMMC_TEST_WRIE_WITH_READ_BACK_CHECK // because eMMC_TEST_WRIE_WITH_READ_BACK_CHECK use bounce buffer to compare data.
	return;
	#endif
	#endif	
	
		
	#ifdef CONFIG_NVT_CHIP
	#ifdef NT72558_eMMC_DEBUG
	*(volatile unsigned long*)(0xBC040204) = 0x72682;
	*(volatile unsigned long*)(0xBC040204) = 0x28627;
	*(volatile unsigned long*)(0xBC040208) = 1;
	*(volatile unsigned long*)(0xBD0F0004) |= (1<<4); //set GPA4 output high.	
	#endif
	#endif

									
	local_irq_save(flags);
	sg_copy_to_buffer(mq->bounce_sg, mq->bounce_sg_len,
		mq->bounce_buf, mq->sg[0].length);
	local_irq_restore(flags);

	
	#ifdef CONFIG_NVT_CHIP
	#ifdef NT72558_eMMC_DEBUG
	*(volatile unsigned long*)(0xBC040204) = 0x72682;
	*(volatile unsigned long*)(0xBC040204) = 0x28627;
	*(volatile unsigned long*)(0xBC040208) = 1;
	*(volatile unsigned long*)(0xBD0F0000) |= (1<<4); //set GPA4 output low.
	#endif
	#endif

	
}

/*
 * If reading, bounce the data from the buffer after the request
 * has been handled by the host driver
 */
void mmc_queue_bounce_post(struct mmc_queue *mq)
{
	unsigned long flags;

	if (!mq->bounce_buf)
		return;


	#ifdef CONFIG_NVT_CHIP
	
	#ifdef eMMC_TEST_WRIE_WITH_READ_BACK_CHECK	
			
	if (rq_data_dir(mq->req) == WRITE)
	{

		unsigned int Backup_of_REG_SDC_INT_ENABLE;
		unsigned int Backup_of_REG_SDC_INT_STAT_ENABLE;

		Backup_of_REG_SDC_INT_ENABLE = REG_SDC_INT_ENABLE;
		Backup_of_REG_SDC_INT_STAT_ENABLE = REG_SDC_INT_STAT_ENABLE;
		REG_SDC_INT_ENABLE = 0;
		REG_SDC_INT_STAT_ENABLE = -1;
	
		if (SDC_ReadSector_Internal(eMMC_readback_check_buf, eMMC_readback_check_dwSectorAddress, 
			eMMC_readback_check_dwSectorCount, eMMC_readback_check_dwSectorSize) == RES_TYPE_SUCCESS)
		{
			unsigned int i;
		
			for (i = 0; i<mq->sg[0].length; i+=4)
			{
				if ( (*(volatile unsigned int*)(eMMC_readback_check_buf+i)) != (*(volatile unsigned int*)(mq->bounce_buf+i)) )
				{
					break;
				}
			}		

			if (i < mq->sg[0].length)
			{
				printk(KERN_WARNING "\n\neMMC_TEST_WRIE_WITH_READ_BACK_CHECK fail!!\n\n");
				while(1) { ; }
			}
		}
		else
		{
			printk(KERN_WARNING "\n\nSDC_ReadSector_Internal fail!\n\n");
		}

		REG_SDC_INT_STAT = -1;  // clear all status
		REG_SDC_INT_STAT_ENABLE = Backup_of_REG_SDC_INT_STAT_ENABLE;
		REG_SDC_INT_ENABLE = Backup_of_REG_SDC_INT_ENABLE;
			
	}
	
	#endif // #ifdef eMMC_TEST_WRIE_WITH_READ_BACK_CHECK	

	
	#ifdef eMMC_REMOVE_DMA_MEMCOPY		
	if (eMMC_DMA_Buffer_Alignment_OK == 1)
	{
		return;
	}
		
	if (rq_data_dir(mq->req) != READ)
		return;
	
	local_irq_save(flags);

	if(mq->sg[0].length)
	{

		unsigned int offset = 0;
		struct sg_mapping_iter miter;		
		//unsigned long Flags;
		unsigned int sg_flags = SG_MITER_ATOMIC;

		sg_flags |= SG_MITER_FROM_SG;
		sg_miter_start(&miter, mq->bounce_sg, mq->bounce_sg_len, sg_flags);

		//local_irq_save(Flags);

		while (sg_miter_next(&miter) && offset < mq->sg[0].length)
		{
    	      		unsigned int len;              		

    	      		len = min(miter.length, mq->sg[0].length - offset);
              							
                    memcpy(miter.addr, mq->bounce_buf + offset, len);
                    
              		dma_cache_wback(((unsigned int)(miter.addr)), len);

             		if (((unsigned int)(miter.addr))%NT72558_Cache_Line_Size)
             		{
                  		printk(KERN_WARNING "mmc_queue_bounce_post: cache alignment problem\r\n");
             		}

                    //printk("buf = 0x%08x / 0x%08x offset = 0x%x\r\n",miter.addr, mq->bounce_buf + offset, offset);

              		offset += len;
		}

		sg_miter_stop(&miter);
		//local_irq_restore(Flags);

	}

	local_irq_restore(flags);

	return;

	#endif // #ifdef eMMC_REMOVE_DMA_MEMCOPY
	
	#endif // #ifdef CONFIG_NVT_CHIP


	if (rq_data_dir(mq->req) != READ)
		return;


#ifdef CONFIG_NVT_CHIP

#ifdef KEEP_ORIGINAL_eMMC_READ

	local_irq_save(flags);

	if(mq->sg[0].length)
	{

		unsigned int offset = 0;
		struct sg_mapping_iter miter;		
		//unsigned long Flags;
		unsigned int sg_flags = SG_MITER_ATOMIC;

		sg_flags |= SG_MITER_FROM_SG;
		sg_miter_start(&miter, mq->bounce_sg, mq->bounce_sg_len, sg_flags);

		//local_irq_save(Flags);

		while (sg_miter_next(&miter) && offset < mq->sg[0].length)
		{
    	      		unsigned int len;              		

    	      		len = min(miter.length, mq->sg[0].length - offset);
              							
                    memcpy(miter.addr, mq->bounce_buf + offset, len);
                    
              		dma_cache_wback(((unsigned int)(miter.addr)), len);

             		if (((unsigned int)(miter.addr))%NT72558_Cache_Line_Size)
             		{
                  		printk(KERN_WARNING "mmc_queue_bounce_post: cache alignment problem\r\n");
             		}

                    //printk("buf = 0x%08x / 0x%08x offset = 0x%x\r\n",miter.addr, mq->bounce_buf + offset, offset);

              		offset += len;
		}

		sg_miter_stop(&miter);
		//local_irq_restore(Flags);

	}

	local_irq_restore(flags);

#else // #ifdef KEEP_ORIGINAL_eMMC_READ
	
	local_irq_save(flags);
	
	if (nvt_eMMC_read_copied_length < mq->sg[0].length)
	{		
		sg_copy_from_buffer_for_nvt_eMMC(nvt_eMMC_mq->bounce_sg, nvt_eMMC_mq->bounce_sg_len,
					nvt_eMMC_mq->bounce_buf, nvt_eMMC_mq->sg[0].length, nvt_eMMC_read_copied_length, (nvt_eMMC_mq->sg[0].length - nvt_eMMC_read_copied_length));
	}

	local_irq_restore(flags);
	
	#ifdef eMMC_DMA_MEMCOPY_USE_INT_FOR_READ		
	if ( ((nvt_eMMC_SDC_INT_DMA_INT_Counter + 1) * eMMC_DMA_BUFFER_BOUND_SIZE_For_READ) < mq->sg[0].length )
	{
		unsigned int waiting_loop_counter = 0;
				
		while(1)
		{						
			udelay(1);
			
			if ( ((nvt_eMMC_SDC_INT_DMA_INT_Counter + 1) * eMMC_DMA_BUFFER_BOUND_SIZE_For_READ) >= mq->sg[0].length )
			{
				break;
			}

			waiting_loop_counter++;

			if (waiting_loop_counter > 200000) // at most wait for 0.2 Second.
			{
				printk(KERN_WARNING "mmc_queue_bounce_post: sdhci_tasklet_dma_boundary not running OK yet!\r\n");
				break;
			}
		}		
	}
	#endif // #ifdef eMMC_DMA_MEMCOPY_USE_INT_FOR_READ

#endif // #else // #ifdef KEEP_ORIGINAL_eMMC_READ

#else // #ifdef CONFIG_NVT_CHIP

    local_irq_save(flags);

	sg_copy_from_buffer(mq->bounce_sg, mq->bounce_sg_len,
		mq->bounce_buf, mq->sg[0].length);

	local_irq_restore(flags);

#endif // #else // #ifdef CONFIG_NVT_CHIP

}


#ifdef CONFIG_NVT_CHIP

#ifdef eMMC_DMA_MEMCOPY_USE_INT_FOR_READ
void Handle_nvt_eMMC_Mem_Copy(void)
{
	if ((nvt_eMMC_SDC_INT_DMA_INT_Counter*eMMC_DMA_BUFFER_BOUND_SIZE_For_READ) < (nvt_eMMC_mq->sg[0].length))
	{
		sg_copy_from_buffer_for_nvt_eMMC(nvt_eMMC_mq->bounce_sg, nvt_eMMC_mq->bounce_sg_len,
			nvt_eMMC_mq->bounce_buf, nvt_eMMC_mq->sg[0].length, nvt_eMMC_read_copied_length, eMMC_DMA_BUFFER_BOUND_SIZE_For_READ);

		nvt_eMMC_read_copied_length = nvt_eMMC_SDC_INT_DMA_INT_Counter*eMMC_DMA_BUFFER_BOUND_SIZE_For_READ;
	}
}

EXPORT_SYMBOL(Handle_nvt_eMMC_Mem_Copy);
#endif // #ifdef eMMC_DMA_MEMCOPY_USE_INT_FOR_READ

#ifdef eMMC_DMA_MEMCOPY_USE_POLLING
void nvt_eMMC_memcopy_for_read(void)
{

	if (rq_data_dir(nvt_eMMC_mq->req) != READ)
		return;
	
	while( REG_SDC_DMA_ADDR < (Memory_Copy_DMA_Start_Address + nvt_eMMC_mq->sg[0].length) )
	{
		if (REG_SDC_DMA_ADDR >= (Memory_Copy_DMA_Start_Address + nvt_eMMC_read_copied_length + eMMC_DMA_MEMCOPY_SIZE))
		{

			#ifdef NT72558_eMMC_DEBUG
			*(volatile unsigned long*)(0xBC040204) = 0x72682;
		    *(volatile unsigned long*)(0xBC040204) = 0x28627;
		    *(volatile unsigned long*)(0xBC040208) = 1;
		    *(volatile unsigned long*)(0xBD0F0004) |= (1<<4); //set GPA4 output high.
		    #endif
    
			////test to remark the following
			//doing memory copy
			sg_copy_from_buffer_for_nvt_eMMC(nvt_eMMC_mq->bounce_sg, nvt_eMMC_mq->bounce_sg_len,
				nvt_eMMC_mq->bounce_buf, nvt_eMMC_mq->sg[0].length, nvt_eMMC_read_copied_length, eMMC_DMA_MEMCOPY_SIZE);

			nvt_eMMC_read_copied_length += eMMC_DMA_MEMCOPY_SIZE;

			//printk(KERN_WARNING "\n\nnvt_eMMC_read_copied_length = 0x%x\n\n", nvt_eMMC_read_copied_length);

			#ifdef NT72558_eMMC_DEBUG
			*(volatile unsigned long*)(0xBC040204) = 0x72682;
			*(volatile unsigned long*)(0xBC040204) = 0x28627;
		    *(volatile unsigned long*)(0xBC040208) = 1;
		    *(volatile unsigned long*)(0xBD0F0000) |= (1<<4); //set GPA4 output low.
		    #endif
    
		}

		if ((nvt_eMMC_read_copied_length + eMMC_DMA_MEMCOPY_SIZE) >= nvt_eMMC_mq->sg[0].length)
		{
			break;
		}
	}

}

EXPORT_SYMBOL(nvt_eMMC_memcopy_for_read);
#endif // #ifdef eMMC_DMA_MEMCOPY_USE_POLLING

#ifndef KEEP_ORIGINAL_eMMC_READ
static size_t sg_copy_buffer_for_nvt_eMMC(struct scatterlist *sgl, unsigned int nents,
			     void *buf, size_t buflen, int to_buffer, size_t start_offset, size_t copy_length)
{
	unsigned int offset = 0;
	struct sg_mapping_iter miter;
	unsigned long flags;
	unsigned int sg_flags = SG_MITER_ATOMIC;
	
	unsigned int had_copied_length;

	if (to_buffer)
		sg_flags |= SG_MITER_FROM_SG;
	else
		sg_flags |= SG_MITER_TO_SG;

	sg_miter_start(&miter, sgl, nents, sg_flags);

	local_irq_save(flags);

	had_copied_length = 0;
	
	while (sg_miter_next(&miter) && offset < buflen) {
		unsigned int len;

		len = min(miter.length, buflen - offset);

		if (to_buffer)
			memcpy(buf + offset, miter.addr, len);
		else			
			{
									
				if ((offset+len) <= start_offset)
				{
					//nogthing to be done.
				}
				else // (offset+len) > start_offset
				{
					if (offset <= start_offset)
					{
						if ((offset+len) <= (start_offset+copy_length))
						{
							memcpy(miter.addr+start_offset-offset, buf+start_offset, len-(start_offset-offset));
							had_copied_length += len-(start_offset-offset);

							dma_cache_wback(((unsigned int)(miter.addr))+start_offset-offset, len-(start_offset-offset));

							if (((unsigned int)(miter.addr)+start_offset-offset)%NT72558_Cache_Line_Size)
		             		{
		                  		printk(KERN_WARNING"sg_copy_buffer_for_nvt_eMMC: cache alignment problem 1!\r\n");
        		     		}
						}
						else // ((offset+len) > (start_offset+copy_length))
						{
							memcpy(miter.addr+start_offset-offset, buf+start_offset, copy_length);
							had_copied_length += copy_length;

							dma_cache_wback(((unsigned int)(miter.addr))+start_offset-offset, copy_length);

							if (((unsigned int)(miter.addr)+start_offset-offset)%NT72558_Cache_Line_Size)
		             		{
		                  		printk(KERN_WARNING"sg_copy_buffer_for_nvt_eMMC: cache alignment problem 2!\r\n");
        		     		}
						}														
					}
					else // (offset > start_offset)
					{
						if (offset >= (start_offset+copy_length))
						{
							//nogthin to be done.
						}
						else // (offset < (start_offset+copy_length))
						{
							if ((offset+len) <= (start_offset+copy_length))
							{
								memcpy(miter.addr, buf + offset, len);
								had_copied_length += len;

								dma_cache_wback(((unsigned int)(miter.addr)), len);

								if (((unsigned int)(miter.addr))%NT72558_Cache_Line_Size)
			             		{
			                  		printk(KERN_WARNING"sg_copy_buffer_for_nvt_eMMC: cache alignment problem 3!\r\n");
        			     		}
							}
							else // ((offset+len) > (start_offset+copy_length))
							{
								memcpy(miter.addr, buf + offset, copy_length-had_copied_length);
								had_copied_length += copy_length-had_copied_length;

								dma_cache_wback(((unsigned int)(miter.addr)), copy_length-had_copied_length);

								if (((unsigned int)(miter.addr))%NT72558_Cache_Line_Size)
			             		{
			                  		printk(KERN_WARNING"sg_copy_buffer_for_nvt_eMMC: cache alignment problem 4!\r\n");
        			     		}
							}
						}
					}
				}									
			}

		offset += len;
	}


	sg_miter_stop(&miter);

	local_irq_restore(flags);
	return offset;
}

size_t sg_copy_from_buffer_for_nvt_eMMC(struct scatterlist *sgl, unsigned int nents,
			 void *buf, size_t buflen, size_t start_offset, size_t copy_length)
{
	return sg_copy_buffer_for_nvt_eMMC(sgl, nents, buf, buflen, 0, start_offset, copy_length);
}
#endif // #ifndef KEEP_ORIGINAL_eMMC_READ

#ifdef eMMC_TEST_WRIE_WITH_READ_BACK_CHECK
int SDC_ReadSector_Internal(char* dwBuffer, DWORD dwSectorAddress, DWORD dwSectorCount, DWORD dwSectorSize)
{   // SD,MMC read sector
	
	DWORD dwCardStatus;
	//DWORD Target_Buffer;
	unsigned int timeout_counter = 0;


	if (!high_capacity)
	{
		dwSectorAddress = dwSectorAddress/512;
	}
	

	if ((((unsigned int)(dwBuffer)) >= 0x80000000) && (((unsigned int)(dwBuffer)) <= 0x9fffffff))
	{

		if ( dwSectorSize == 512 )
		{
			if ((((unsigned int)(dwBuffer))%NT72558_Cache_Line_Size) == 0)
			{
				dma_cache_inv(((unsigned int)(dwBuffer)), (dwSectorCount*512));				
			}
			else
			{								
				printk(KERN_WARNING "\n\nSDC_ReadSector_Internal : 1 dwBuffer= 0x%x\n\n", ((unsigned int)(dwBuffer)));
			}
		}
		else
		{
			printk(KERN_WARNING "\n\nWarning : SDC_ReadSector_Internal with dwSectorSize != 512 !!\n\n");

			if (((((unsigned int)(dwBuffer))%NT72558_Cache_Line_Size) == 0) && (((dwSectorCount*dwSectorSize)%NT72558_Cache_Line_Size) == 0))
			{
				dma_cache_inv(((unsigned int)(dwBuffer)), (dwSectorCount*dwSectorSize));				
			}
			else
			{
				printk(KERN_WARNING "\n\nSDC_ReadSector_Internal : 2 dwBuffer= 0x%x\n\n", ((unsigned int)(dwBuffer)));
			}
		}

	}
	else
	{
		printk(KERN_WARNING "\n\nWarning : SDC_ReadSector_Internal with dwBuffer not inside D-Cache area !!\n\n");

		printk(KERN_WARNING "\n\nSDC_ReadSector_Internal : 3 dwBuffer= 0x%x\n\n", ((unsigned int)(dwBuffer)));
	}


	// wait for cmd & data line available
	while(REG_SDC_STAT & (SDC_STAT_CMD_INHIBIT_CMD | SDC_STAT_CMD_INHIBIT_DAT))
	{
		if(!DRV_SDC_CardStatus_Inserted())
		{   // card removed			
			printk(KERN_WARNING "\n\nSDC_ReadSector_Internal : DRV_SDC_CardStatus_Inserted detect fail 1!\n\n");
			
			return RES_TYPE_ERR;
		}
		
		printk(KERN_WARNING "\n\nSDC_ReadSector_Internal : wait for cmd & data line available...\n\n");		
	}

	// CMD18: READ_MULTIPLE_BLOCK(R1)
	REG_SDC_BLK_SIZE = SDC_BLK_SIZE_TRAN_BLK_SIZE(512) | SDC_BLK_SIZE_DMA_BUFF_BND_512K;    // block(sector) size is 512 bytes
	REG_SDC_BLK_COUNT = dwSectorCount;
	
	REG_SDC_DMA_ADDR = ((unsigned int)(dwBuffer)) & 0x1FFFFFFF;

	REG_SDC_TRAN_MODE = SDC_TRAN_MODE_MULT_BLK | SDC_TRAN_MODE_BLK_COUNT | SDC_TRAN_MODE_AUTO_CMD12 | SDC_TRAN_MODE_READ | SDC_TRAN_MODE_DMA;

	REG_SDC_ARG = dwSectorAddress * (high_capacity? 1: 512);   // addr in byte units for standard, in block (sector) units for high capacity

	REG_SDC_INT_STAT = -1;  // clear all status
	REG_SDC_CMD = SDC_CMD_IDX(18) | SDC_CMD_IDX_CHK | SDC_CMD_CRC_CHK | SDC_CMD_RESP_TYPE_LEN_48 | SDC_CMD_DATA_PRESENT;    // send command

	// wait for command complete
	while(!(REG_SDC_INT_STAT & SDC_INT_CMD_COMPLETE))
	{
		if(REG_SDC_INT_STAT & SDC_INT_ERR_INT)
		{			
			printk(KERN_WARNING "\n\nSDC_ReadSector_Internal : REG_SDC_INT_STAT & SDC_INT_ERR_INT detect fail 1!\n\n");
			
			return RES_TYPE_ERR;
		}

		if(!DRV_SDC_CardStatus_Inserted())
		{   // card removed			
			printk(KERN_WARNING "\n\nSDC_ReadSector_Internal : DRV_SDC_CardStatus_Inserted detect fail 2!\n\n");
			
			return RES_TYPE_ERR;
		}
		
		udelay(1);
		timeout_counter++;
		if (timeout_counter > 500000)
		{					
			printk(KERN_WARNING "\n\nSDC_ReadSector_Internal : timeout_counter > 500000!\n\n");
			
			return RES_TIMEOUT_ERR;
		}

	}

	// check response R1 (card status)
	dwCardStatus = REG_SDC_RESP0;
	if(dwCardStatus & 0xfdf90008)
	{   // error card status		
		printk(KERN_WARNING "\n\nSDC_ReadSector_Internal : dwCardStatus & 0xfdf90008 detect fail 1!\n\n");
		
		return RES_TYPE_ERR;
	}

	// wait for transfer complete
	timeout_counter = 0;
	while(!(REG_SDC_INT_STAT & SDC_INT_TRAN_COMPLETE))
	{
		if(REG_SDC_INT_STAT & SDC_INT_DMA_INT)
		{   // dma int occured. set dma address to continue.
			REG_SDC_INT_STAT = SDC_INT_DMA_INT;
			REG_SDC_DMA_ADDR = REG_SDC_DMA_ADDR;
			continue;
		}

		if(REG_SDC_INT_STAT & SDC_INT_ERR_INT)
		{   // error int occured. assume it's line signal error.			
			printk(KERN_WARNING "\n\nSDC_ReadSector_Internal : REG_SDC_INT_STAT & SDC_INT_ERR_INT detect fail 2!\n\n");
			
			return RES_TYPE_ERR;
		}

		if(!DRV_SDC_CardStatus_Inserted())
		{   // card removed			
			printk(KERN_WARNING "\n\nSDC_ReadSector_Internal : DRV_SDC_CardStatus_Inserted detect fail 3!\n\n");
			
			return RES_TYPE_ERR;
		}		
		
		udelay(1);
		timeout_counter++;
		if (timeout_counter > 2000000)
		{			
			printk(KERN_WARNING "\n\nSDC_ReadSector_Internal : timeout_counter > 2000000 ~ 1!\n\n");
			
			return RES_TIMEOUT_ERR;
		}
		
	}

	// check card status and wait current state becomes tran state
	timeout_counter = 0;
	do
	{
		// get card status
		dwCardStatus = SDC_CMD13();

		if(dwCardStatus == 0)
		{   // get card status fail. assume it's line signal error.			
			printk(KERN_WARNING "\n\nSDC_ReadSector_Internal : dwCardStatus == 0 returned by SDC_CMD13!\n\n");
			
			return RES_TYPE_ERR;
		}

		if(dwCardStatus & 0xfdf90008)
		{   // error card status			
			printk(KERN_WARNING "\n\nSDC_ReadSector_Internal : dwCardStatus & 0xfdf90008 detect fail 2!\n\n");
			
			return RES_TYPE_ERR;
		}

		if(REG_SDC_INT_STAT & SDC_INT_ERR_INT)
		{   // error int occured. assume it's line signal error.			
			printk(KERN_WARNING "\n\nSDC_ReadSector_Internal : REG_SDC_INT_STAT & SDC_INT_ERR_INT detect fail 3!\n\n");
			
			return RES_TYPE_ERR;
		}

		if(!DRV_SDC_CardStatus_Inserted())
		{   // card removed			
			printk(KERN_WARNING "\n\nSDC_ReadSector_Internal : DRV_SDC_CardStatus_Inserted detect fail 4!\n\n");
			
			return RES_TYPE_ERR;
		}
		
		udelay(1);
		timeout_counter++;
		if (timeout_counter > 2000000)
		{
			printk(KERN_WARNING "\n\nSDC_ReadSector_Internal : timeout_counter > 2000000 ~ 2!\n\n");
			
			return RES_TIMEOUT_ERR;
		}
		
	}
	while((dwCardStatus & (0xf << 9)) != (4 << 9)); // loop until current state is tran
	
	return RES_TYPE_SUCCESS;

}

bool DRV_SDC_CardStatus_Inserted(void)
{
	return (REG_SDC_STAT & SDC_STAT_CARD_INS) != 0;
}

DWORD SDC_CMD13(void)
{   // CMD13: SEND_STATUS(R1)

	unsigned int timeout_counter = 0;
	
	if((REG_SDC_INT_STAT & SDC_INT_ERR_INT) == 0)
	{		
		REG_SDC_INT_STAT = SDC_INT_CMD_COMPLETE;    // clear command complete status
		REG_SDC_ARG = dwRCA;    // ARG: RCA
				
		REG_SDC_CMD = SDC_CMD_IDX(13) | SDC_CMD_IDX_CHK | SDC_CMD_CRC_CHK | SDC_CMD_RESP_TYPE_LEN_48;   // send command
		while(!(REG_SDC_INT_STAT & (SDC_INT_CMD_COMPLETE | SDC_INT_ERR_INT)))   // wait for command complete or error
		{			
			udelay(1);
			timeout_counter++;
			if (timeout_counter > 250000)
			{				
				printk(KERN_WARNING "\n\nSDC_CMD13 : timeout_counter > 250000!\n\n");
				
				return 0;
			}
		}
		if((REG_SDC_INT_STAT & SDC_INT_ERR_INT) == 0)
		{						
			return REG_SDC_RESP0;
		}
	}
	
	printk(KERN_WARNING "\n\nSDC_CMD13 : end with return 0!\n\n");
	
	return 0;
	
}
#endif // #ifdef eMMC_TEST_WRIE_WITH_READ_BACK_CHECK

#ifdef eMMC_REMOVE_DMA_MEMCOPY
int Arrange_eMMC_DMA_Buffer(char* buf_1, char* buf, int DMA_buf_count)
{
	unsigned int i, j, k;	
	unsigned int DMA_Address;
	unsigned int DMA_Length;
	unsigned int DMA_Address_1;	
	unsigned int DMA_Address_Array[MMC_QUEUE_BOUNCESZ/4096];
	unsigned int DMA_Length_Array[MMC_QUEUE_BOUNCESZ/4096];
	int DMA_Arrange_Counter = 0;

	
	eMMC_DMA_Buffer_Alignment_OK = 1;
	
	for (i=0; i<DMA_buf_count; i++)
	{
		DMA_Address = *(volatile unsigned int*)(buf+i*8);
		DMA_Length = *(volatile unsigned int*)(buf+i*8+4);

		
		if (DMA_Address % (4*1024))
		{
			//printk(KERN_WARNING "scatterlist buffer start address is not aligned to 4KBytes! DMA_Address = 0x%x, DMA_Length = 0x%x, i = 0x%x\n\n", DMA_Address, DMA_Length, i);

			eMMC_DMA_Buffer_Alignment_OK = 0;

			return 0;
		}
		if (DMA_Length % (4*1024))
		{
			//printk(KERN_WARNING "scatterlist buffer length is not integer times of 4KBytes!  DMA_Address = 0x%x, DMA_Length = 0x%x, i = 0x%x\n\n", DMA_Address, DMA_Length, i);

			eMMC_DMA_Buffer_Alignment_OK = 0;

			return 0;
		}
				

		DMA_Address_1 = DMA_Address;
		j = 0;
		while(1)
		{

			DMA_Address_Array[j] = DMA_Address_1;

			if (DMA_Length >= 4*1024)
			{
				DMA_Length = DMA_Length - 4*1024;
				DMA_Length_Array[j] = 4*1024;
				DMA_Address_1 += 4*1024;				
			}
			else
			{	
				if (DMA_Length != 0)
				{
					printk(KERN_ERR "Arrange_eMMC_DMA_Buffer : DMA_Length handling error 1!\n\n");
					
					eMMC_DMA_Buffer_Alignment_OK = 0;

					return 0;
				}				
			}
						
			j++;
				
			if (DMA_Length < 4*1024)
			{	
				if (DMA_Length != 0)
				{
					printk(KERN_ERR "Arrange_eMMC_DMA_Buffer : DMA_Length handling error 2!\n\n");
					
					eMMC_DMA_Buffer_Alignment_OK = 0;

					return 0;
				}
				
				break;
			}
			
		}

		for (k=0; k<j; k++)
		{
			*(volatile unsigned int*)(buf_1+ DMA_Arrange_Counter*8 + k*8) = DMA_Address_Array[k];
		    *(volatile unsigned int*)(buf_1+ DMA_Arrange_Counter*8 + k*8 + 4) = DMA_Length_Array[k];
		}

		DMA_Arrange_Counter += j;
		
	}

	return DMA_Arrange_Counter;

}
EXPORT_SYMBOL(Arrange_eMMC_DMA_Buffer);

void mmc_queue_bounce_pre_redo(struct mmc_queue *mq)
{
	unsigned long flags;


	#ifdef eMMC_TEST_WRIE_WITH_READ_BACK_CHECK // already done in "void mmc_queue_bounce_pre(struct mmc_queue *mq)".
	return;
	#endif

	
	if (!mq->bounce_buf)
		return;

	if (rq_data_dir(mq->req) != WRITE)
		return;
				
		
	#ifdef CONFIG_NVT_CHIP
	#ifdef NT72558_eMMC_DEBUG
	*(volatile unsigned long*)(0xBC040204) = 0x72682;
	*(volatile unsigned long*)(0xBC040204) = 0x28627;
	*(volatile unsigned long*)(0xBC040208) = 1;
	*(volatile unsigned long*)(0xBD0F0004) |= (1<<4); //set GPA4 output high.	
	#endif
	#endif

									
	local_irq_save(flags);
	sg_copy_to_buffer(mq->bounce_sg, mq->bounce_sg_len,
		mq->bounce_buf, mq->sg[0].length);
	local_irq_restore(flags);

	
	#ifdef CONFIG_NVT_CHIP
	#ifdef NT72558_eMMC_DEBUG
	*(volatile unsigned long*)(0xBC040204) = 0x72682;
	*(volatile unsigned long*)(0xBC040204) = 0x28627;
	*(volatile unsigned long*)(0xBC040208) = 1;
	*(volatile unsigned long*)(0xBD0F0000) |= (1<<4); //set GPA4 output low.
	#endif
	#endif

	
}
#endif // #ifdef eMMC_REMOVE_DMA_MEMCOPY

#endif // #ifdef CONFIG_NVT_CHIP


