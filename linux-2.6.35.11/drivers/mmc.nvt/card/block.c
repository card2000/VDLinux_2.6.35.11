/*
 * Block driver for media (i.e., flash cards)
 *
 * Copyright 2002 Hewlett-Packard Company
 * Copyright 2005-2008 Pierre Ossman
 *
 * Use consistent with the GNU GPL is permitted,
 * provided that this copyright notice is
 * preserved in its entirety in all copies and derived works.
 *
 * HEWLETT-PACKARD COMPANY MAKES NO WARRANTIES, EXPRESSED OR IMPLIED,
 * AS TO THE USEFULNESS OR CORRECTNESS OF THIS CODE OR ITS
 * FITNESS FOR ANY PARTICULAR PURPOSE.
 *
 * Many thanks to Alessandro Rubini and Jonathan Corbet!
 *
 * Author:  Andrew Christian
 *          28 May 2002
 */
#include <linux/moduleparam.h>
#include <linux/module.h>
#include <linux/init.h>

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/hdreg.h>
#include <linux/kdev_t.h>
#include <linux/blkdev.h>
#include <linux/mutex.h>
#include <linux/scatterlist.h>
#include <linux/string_helpers.h>

#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>

#include <asm/system.h>
#include <asm/uaccess.h>

#include "queue.h"
#include <linux/delay.h>

#include "../core/mmc_ops.h"


#ifdef CONFIG_NVT_CHIP
#include "../host/nt72558_mmc.h"

#ifndef KEEP_ORIGINAL_eMMC_READ
volatile struct mmc_queue *nvt_eMMC_mq;
volatile unsigned int nvt_eMMC_read_copied_length = 0;
EXPORT_SYMBOL(nvt_eMMC_mq);
EXPORT_SYMBOL(nvt_eMMC_read_copied_length);

volatile unsigned int mmc_blk_issue_rq_running = 0;
EXPORT_SYMBOL(mmc_blk_issue_rq_running);

volatile unsigned int mmc_blk_issue_rq_running_for_write = 0;
EXPORT_SYMBOL(mmc_blk_issue_rq_running_for_write);
#endif // #ifndef KEEP_ORIGINAL_eMMC_READ

#ifdef eMMC_DMA_MEMCOPY_USE_INT_FOR_READ
volatile unsigned int nvt_eMMC_SDC_INT_DMA_INT_Counter = 0;
EXPORT_SYMBOL(nvt_eMMC_SDC_INT_DMA_INT_Counter);
#endif // #ifdef eMMC_DMA_MEMCOPY_USE_INT_FOR_READ

#ifdef eMMC_DMA_MEMCOPY_USE_POLLING
volatile unsigned int Memory_Copy_DMA_Start_Address;
EXPORT_SYMBOL(Memory_Copy_DMA_Start_Address);
#endif // #ifdef eMMC_DMA_MEMCOPY_USE_POLLING
#endif // #ifdef CONFIG_NVT_CHIP


MODULE_ALIAS("mmc:block");

/*
 * max 32 partitions per card
 */
/* selp patch by namjae.jeon@samsung.com 
   currently number of partitions is restrict by 8 in mmc device.
   I try to modify 8 -> 32 partitions by needs of project team. 
*/
#define MMC_SHIFT  5
#define MMC_NUM_MINORS	(256 >> MMC_SHIFT)

#define GET_SMART_REPORT 0x5627
#define RESTORE_BOOT_PARTITION 0x5628
#define RESTORE_PARTITION 0x5629
#define VENDOR_CMD_ARG 0xEFAC62EC
#define OPEN_CMD_ARG 0xCCEE
#define CLOSE_CMD_ARG 0xDECCEE
#define SECTOR_SIZE 512 

typedef struct 
{
	char buf[PAGE_SIZE];
	unsigned int offset;
}_mmc_arg;

struct mmc_card *ioctl_card;
unsigned int high_capacity;
static DECLARE_BITMAP(dev_use, MMC_NUM_MINORS);

/*
 * There is one mmc_blk_data per slot.
 */
struct mmc_blk_data {
	spinlock_t	lock;
	struct gendisk	*disk;
	struct mmc_queue queue;

	unsigned int	usage;
	unsigned int	read_only;
};

static DEFINE_MUTEX(open_lock);

static int mmc_send_smart_report_cmd(struct mmc_card *card, u32 arg)
{
	struct mmc_command cmd;
	int err;
	memset(&cmd, 0, sizeof(struct mmc_command));

	cmd.opcode = MMC_VERDOR_CMD;
	cmd.arg = arg;
	cmd.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;

	err = mmc_wait_for_cmd(card->host, &cmd, 0);
	if (err) {
		printk("mmc_send_smart_report_cmd error \n");
		return err;
		}
	mdelay(100);
	return 0;
}

static int mmc_read_smart_report(struct mmc_card *card, struct  mmc_host *host, void *buf)
{
	struct mmc_request mrq;
	struct mmc_command cmd;
	struct mmc_data data;
	struct scatterlist sg;
	void *data_buf;

	data_buf = kmalloc(512, GFP_KERNEL);
	if (data_buf == NULL)
		return -ENOMEM;

	memset(&mrq, 0, sizeof(struct mmc_request));
	memset(&cmd, 0, sizeof(struct mmc_command));
	memset(&data, 0, sizeof(struct mmc_data));

	mrq.cmd = &cmd;
	mrq.data = &data;

	cmd.opcode = MMC_READ_SINGLE_BLOCK;
	cmd.arg = 0;

	cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;

	data.blksz = 512;
	data.blocks = 1;
	data.flags = MMC_DATA_READ;
	data.sg = &sg;
	data.sg_len = 1;

	sg_init_one(&sg, data_buf, 512);

	mmc_set_data_timeout(&data, card);

	mmc_wait_for_req(host, &mrq);
	
	memcpy(buf, data_buf, 512);
	kfree(data_buf);

	if (cmd.error) {
		printk("mmc cmd error !! %u\n", cmd.error);
		return cmd.error;
		}
	if (data.error) {
		printk("mmc data error !! %u\n", data.error);
		return data.error;
		}

	return 0;
}

/*
 * Wait for the card to finish the busy state
 */
static int mmc_wait_busy(struct mmc_card *card)
{
	int ret, busy;
	struct mmc_command cmd;

	busy = 0;
	do {
		memset(&cmd, 0, sizeof(struct mmc_command));

		cmd.opcode = MMC_SEND_STATUS;
		cmd.arg = card->rca << 16;
		cmd.flags = MMC_RSP_R1 | MMC_CMD_AC;

		ret = mmc_wait_for_cmd(card->host, &cmd, 0);
		if (ret)
			break;

		if (!busy && !(cmd.resp[0] & R1_READY_FOR_DATA)) {
			busy = 1;
		}
	} while (!(cmd.resp[0] & R1_READY_FOR_DATA));

	return ret;
}

static int mmc_restore_partition(struct mmc_card *card, void *buf, unsigned int offset, unsigned int rw_cmd)
{
	struct mmc_request mrq;
	struct mmc_command cmd;
	struct mmc_command stop;
	struct mmc_data data;
	struct scatterlist sg;


	memset(&mrq, 0, sizeof(struct mmc_request));
	memset(&cmd, 0, sizeof(struct mmc_command));
	memset(&data, 0, sizeof(struct mmc_data));
	memset(&stop, 0, sizeof(struct mmc_command));

	mrq.cmd = &cmd;
	mrq.stop = &stop;
	mrq.data = &data;
	
	cmd.arg = offset;
	cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;
	data.blksz = 1 << 9;
	stop.opcode = MMC_STOP_TRANSMISSION;
	stop.arg = 0;
	stop.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;
	data.blocks = 8;

    if(rw_cmd == READ)
    {
        data.flags |= MMC_DATA_READ;
        if (data.blocks > 1)
        {
            cmd.opcode = MMC_READ_MULTIPLE_BLOCK;
        }
        else
        {
            cmd.opcode = MMC_READ_SINGLE_BLOCK;
        }
    }
    else if(rw_cmd == WRITE)
    {
        data.flags |= MMC_DATA_WRITE;
        if (data.blocks > 1)
        {
            cmd.opcode = MMC_WRITE_MULTIPLE_BLOCK;
        }
        else
        {
            cmd.opcode = MMC_WRITE_BLOCK;
        }        
    }
    else 
    {
        printk("no read/write cmd error\n");
        return 0;
    }
    
	mmc_set_data_timeout(&data, card);

	data.sg = &sg;
	data.sg_len = 1;

	sg_init_one(&sg, buf, PAGE_SIZE);
	
	mmc_wait_for_req(card->host, &mrq);

	if (cmd.error) {
		printk(KERN_ERR "error %d sending read/write command\n",
				cmd.error);
	}

	if (data.error) {
		printk(KERN_ERR "error %d transferring data\n",
				 data.error);
	}

	if (stop.error) {
		printk(KERN_ERR "error %d sending stop command\n",
				stop.error);
	}

	
	mmc_wait_busy(card);

	return 0;
}

static int mmc_boot_part_enable(struct mmc_card *card)
{
	int err = 0;
	
	err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
			BOOT_CONFIG,
			BOOT_CONFIG_ENABLE);

	if(err) {
		printk("BOOT_CONFIG_ENABLE error\n");
		return err;
	}
	mmc_wait_busy(card);

	err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
			BOOT_BUS_WIDTH,
			BOOT_BUS_WIDTH_8);
	if (err) {
		printk("BOOT_BUS_WIDTH_8 error\n");
		return err;
	}

	mmc_wait_busy(card);
	return err;
}

static int mmc_boot_part_disable(struct mmc_card *card)
{
	int err = 0;
	err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
			BOOT_CONFIG,
			BOOT_CONFIG_DISABLE);
	
	if(err) {
		printk("BOOT_CONFIG_DISABLE error\n");
		return err;
	}

	mmc_wait_busy(card);
	return err;
}

/* Written by namjae.jeon@samsung.com 
   I make update routine to restore boot and normal partition throughtout ioctl. */

static int mmc_ioctl(struct block_device *bdev, fmode_t mode, unsigned int cmd, unsigned long arg)
{
	switch(cmd)
	{
		case GET_SMART_REPORT:
		{
			void *buf;
			if(!(buf = kmalloc(512,GFP_KERNEL))) {
					return -ENOMEM;
			}
			u32 vendor_cmd_arg = VENDOR_CMD_ARG;
			u32 open_cmd_arg = OPEN_CMD_ARG;
			u32 close_cmd_arg = CLOSE_CMD_ARG;

			mmc_claim_host(ioctl_card->host);

			mmc_send_smart_report_cmd(ioctl_card,vendor_cmd_arg);
			mmc_send_smart_report_cmd(ioctl_card,open_cmd_arg);
			mmc_read_smart_report(ioctl_card, ioctl_card->host, buf);
			if(copy_to_user((void *) arg,buf, 512)) {
				kfree(buf);
				return -EFAULT;
			}
			mmc_send_smart_report_cmd(ioctl_card,vendor_cmd_arg);
			mmc_send_smart_report_cmd(ioctl_card,close_cmd_arg);
			mmc_release_host(ioctl_card->host);
			kfree(buf);
			break;
		}
		case RESTORE_BOOT_PARTITION:
		{
			_mmc_arg *mmc_arg;
			unsigned int m_offset;
            
			if(!(mmc_arg = kmalloc(sizeof(_mmc_arg), GFP_KERNEL))) {
					return -ENOMEM;
			}
            
			if(copy_from_user(mmc_arg, (void *) arg,sizeof(_mmc_arg) )) {
				kfree(mmc_arg);
				return -EFAULT;
			}
            if(high_capacity)
            {
    			m_offset = (mmc_arg->offset)/SECTOR_SIZE;

            }else{
    			m_offset =  mmc_arg->offset;
            }            
			mmc_claim_host(ioctl_card->host);
			mmc_boot_part_enable(ioctl_card);
			mmc_restore_partition(ioctl_card,mmc_arg->buf,m_offset, WRITE);
			mmc_boot_part_disable(ioctl_card);
			mmc_release_host(ioctl_card->host);
            
			kfree(mmc_arg);
            
			break;
		}
		case RESTORE_PARTITION:
		{
			_mmc_arg *mmc_arg;
			unsigned int m_offset;
			if(!(mmc_arg = kmalloc(sizeof(_mmc_arg), GFP_KERNEL))) {
					return -ENOMEM;
			}
			if(copy_from_user(mmc_arg, (void *) arg,sizeof(_mmc_arg) )) {
				kfree(mmc_arg);
				return -EFAULT;
			}
            if(high_capacity){
    			m_offset =  get_start_sect(bdev) +  mmc_arg->offset/SECTOR_SIZE;

            }else{
    			m_offset =  (get_start_sect(bdev) * SECTOR_SIZE) +  mmc_arg->offset;
            }            
			mmc_claim_host(ioctl_card->host);
			mmc_restore_partition(ioctl_card,mmc_arg->buf,m_offset, WRITE);
			mmc_release_host(ioctl_card->host);
			kfree(mmc_arg);
			break;
		}
	}

	return 0;
}

static struct mmc_blk_data *mmc_blk_get(struct gendisk *disk)
{
	struct mmc_blk_data *md;

	mutex_lock(&open_lock);
	md = disk->private_data;
	if (md && md->usage == 0)
		md = NULL;
	if (md)
		md->usage++;
	mutex_unlock(&open_lock);

	return md;
}

static void mmc_blk_put(struct mmc_blk_data *md)
{
	mutex_lock(&open_lock);
	md->usage--;
	if (md->usage == 0) {
		int devmaj = MAJOR(disk_devt(md->disk));
		int devidx = MINOR(disk_devt(md->disk)) >> MMC_SHIFT;

		if (!devmaj)
			devidx = md->disk->first_minor >> MMC_SHIFT;

		blk_cleanup_queue(md->queue.queue);

		__clear_bit(devidx, dev_use);

		put_disk(md->disk);
		kfree(md);
	}
	mutex_unlock(&open_lock);
}

static int mmc_blk_open(struct block_device *bdev, fmode_t mode)
{
	struct mmc_blk_data *md = mmc_blk_get(bdev->bd_disk);
	int ret = -ENXIO;

	if (md) {
		if (md->usage == 2)
			check_disk_change(bdev);
		ret = 0;

		if ((mode & FMODE_WRITE) && md->read_only) {
			mmc_blk_put(md);
			ret = -EROFS;
		}
	}

	return ret;
}

static int mmc_blk_release(struct gendisk *disk, fmode_t mode)
{
	struct mmc_blk_data *md = disk->private_data;

	mmc_blk_put(md);
	return 0;
}

static int
mmc_blk_getgeo(struct block_device *bdev, struct hd_geometry *geo)
{
	geo->cylinders = get_capacity(bdev->bd_disk) / (4 * 16);
	geo->heads = 4;
	geo->sectors = 16;
	return 0;
}

static const struct block_device_operations mmc_bdops = {
	.open			= mmc_blk_open,
	.release		= mmc_blk_release,
	.getgeo			= mmc_blk_getgeo,
	.ioctl			= mmc_ioctl,
	.owner			= THIS_MODULE,
};

struct mmc_blk_request {
	struct mmc_request	mrq;
	struct mmc_command	cmd;
	struct mmc_command	stop;
	struct mmc_data		data;
};

static u32 mmc_sd_num_wr_blocks(struct mmc_card *card)
{
	int err;
	u32 result;
	__be32 *blocks;

	struct mmc_request mrq;
	struct mmc_command cmd;
	struct mmc_data data;
	unsigned int timeout_us;

	struct scatterlist sg;

	memset(&cmd, 0, sizeof(struct mmc_command));

	cmd.opcode = MMC_APP_CMD;
	cmd.arg = card->rca << 16;
	cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_AC;

	err = mmc_wait_for_cmd(card->host, &cmd, 0);
	if (err)
		return (u32)-1;
	if (!mmc_host_is_spi(card->host) && !(cmd.resp[0] & R1_APP_CMD))
		return (u32)-1;

	memset(&cmd, 0, sizeof(struct mmc_command));

	cmd.opcode = SD_APP_SEND_NUM_WR_BLKS;
	cmd.arg = 0;
	cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;

	memset(&data, 0, sizeof(struct mmc_data));

	data.timeout_ns = card->csd.tacc_ns * 100;
	data.timeout_clks = card->csd.tacc_clks * 100;

	timeout_us = data.timeout_ns / 1000;
	timeout_us += data.timeout_clks * 1000 /
		(card->host->ios.clock / 1000);

	if (timeout_us > 100000) {
		data.timeout_ns = 100000000;
		data.timeout_clks = 0;
	}

	data.blksz = 4;
	data.blocks = 1;
	data.flags = MMC_DATA_READ;
	data.sg = &sg;
	data.sg_len = 1;

	memset(&mrq, 0, sizeof(struct mmc_request));

	mrq.cmd = &cmd;
	mrq.data = &data;

	blocks = kmalloc(4, GFP_KERNEL);
	if (!blocks)
		return (u32)-1;

	sg_init_one(&sg, blocks, 4);

	mmc_wait_for_req(card->host, &mrq);

	result = ntohl(*blocks);
	kfree(blocks);

	if (cmd.error || data.error)
		result = (u32)-1;

	return result;
}

static u32 get_card_status(struct mmc_card *card, struct request *req)
{
	struct mmc_command cmd;
	int err;

	memset(&cmd, 0, sizeof(struct mmc_command));
	cmd.opcode = MMC_SEND_STATUS;
	if (!mmc_host_is_spi(card->host))
		cmd.arg = card->rca << 16;
	cmd.flags = MMC_RSP_SPI_R2 | MMC_RSP_R1 | MMC_CMD_AC;
	err = mmc_wait_for_cmd(card->host, &cmd, 0);
	if (err)
		printk(KERN_ERR "%s: error %d sending status comand",
		       req->rq_disk->disk_name, err);
	return cmd.resp[0];
}

static int mmc_blk_issue_rq(struct mmc_queue *mq, struct request *req)
{
	struct mmc_blk_data *md = mq->data;
	struct mmc_card *card = md->queue.card;
	struct mmc_blk_request brq;
	int ret = 1, disable_multi = 0;

	mmc_claim_host(card->host);

	do {
		struct mmc_command cmd;
		u32 readcmd, writecmd, status = 0;

		memset(&brq, 0, sizeof(struct mmc_blk_request));
		brq.mrq.cmd = &brq.cmd;
		brq.mrq.data = &brq.data;

		brq.cmd.arg = blk_rq_pos(req);
		if (!mmc_card_blockaddr(card))
			brq.cmd.arg <<= 9;
		brq.cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;
		brq.data.blksz = 512;
		brq.stop.opcode = MMC_STOP_TRANSMISSION;
		brq.stop.arg = 0;
		brq.stop.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;
		brq.data.blocks = blk_rq_sectors(req);

		/*
		 * The block layer doesn't support all sector count
		 * restrictions, so we need to be prepared for too big
		 * requests.
		 */
		if (brq.data.blocks > card->host->max_blk_count)
			brq.data.blocks = card->host->max_blk_count;

		/*
		 * After a read error, we redo the request one sector at a time
		 * in order to accurately determine which sectors can be read
		 * successfully.
		 */
		if (disable_multi && brq.data.blocks > 1)
			brq.data.blocks = 1;

		if (brq.data.blocks > 1) {
			/* SPI multiblock writes terminate using a special
			 * token, not a STOP_TRANSMISSION request.
			 */
			if (!mmc_host_is_spi(card->host)
					|| rq_data_dir(req) == READ)
				brq.mrq.stop = &brq.stop;
			readcmd = MMC_READ_MULTIPLE_BLOCK;
			writecmd = MMC_WRITE_MULTIPLE_BLOCK;
		} else {
			brq.mrq.stop = NULL;
			readcmd = MMC_READ_SINGLE_BLOCK;
			writecmd = MMC_WRITE_BLOCK;
		}

		if (rq_data_dir(req) == READ) {
			brq.cmd.opcode = readcmd;
			brq.data.flags |= MMC_DATA_READ;
		} else {
			brq.cmd.opcode = writecmd;
			brq.data.flags |= MMC_DATA_WRITE;
		}

		mmc_set_data_timeout(&brq.data, card);

		brq.data.sg = mq->sg;
		brq.data.sg_len = mmc_queue_map_sg(mq);

		/*
		 * Adjust the sg list so it is the same size as the
		 * request.
		 */
		if (brq.data.blocks != blk_rq_sectors(req)) {
			int i, data_size = brq.data.blocks << 9;
			struct scatterlist *sg;

			for_each_sg(brq.data.sg, sg, brq.data.sg_len, i) {
				data_size -= sg->length;
				if (data_size <= 0) {
					sg->length += data_size;
					i++;
					break;
				}
			}
			brq.data.sg_len = i;
		}
	
		mmc_queue_bounce_pre(mq);


		#ifdef CONFIG_NVT_CHIP
		#ifndef KEEP_ORIGINAL_eMMC_READ
		nvt_eMMC_mq = mq;		
		nvt_eMMC_read_copied_length = 0;
		#endif
		#ifdef eMMC_DMA_MEMCOPY_USE_INT_FOR_READ
		nvt_eMMC_SDC_INT_DMA_INT_Counter = 0;
		#endif
		#ifndef KEEP_ORIGINAL_eMMC_READ
		mmc_blk_issue_rq_running = 1;

		
		if (rq_data_dir(mq->req) == WRITE)
		{
			mmc_blk_issue_rq_running_for_write = 1;
		}
		else
		{
			mmc_blk_issue_rq_running_for_write = 0;
		}
		
		#endif
		#endif // #ifdef CONFIG_NVT_CHIP


		mmc_wait_for_req(card->host, &brq.mrq);


		#ifdef CONFIG_NVT_CHIP
		#ifndef KEEP_ORIGINAL_eMMC_READ
		mmc_blk_issue_rq_running = 0;
		#endif
		#endif // #ifdef CONFIG_NVT_CHIP
		

		mmc_queue_bounce_post(mq);

		/*
		 * Check for errors here, but don't jump to cmd_err
		 * until later as we need to wait for the card to leave
		 * programming mode even when things go wrong.
		 */
		if (brq.cmd.error || brq.data.error || brq.stop.error) {
			if (brq.data.blocks > 1 && rq_data_dir(req) == READ) {
				/* Redo read one sector at a time */
				printk(KERN_WARNING "%s: retrying using single "
				       "block read\n", req->rq_disk->disk_name);
				disable_multi = 1;
				continue;
			}
			status = get_card_status(card, req);
		}

		if (brq.cmd.error) {
			printk(KERN_ERR "%s: error %d sending read/write "
			       "command, response %#x, card status %#x\n",
			       req->rq_disk->disk_name, brq.cmd.error,
			       brq.cmd.resp[0], status);
		}

		if (brq.data.error) {
			if (brq.data.error == -ETIMEDOUT && brq.mrq.stop)
				/* 'Stop' response contains card status */
				status = brq.mrq.stop->resp[0];
			printk(KERN_ERR "%s: error %d transferring data,"
			       " sector %u, nr %u, card status %#x\n",
			       req->rq_disk->disk_name, brq.data.error,
			       (unsigned)blk_rq_pos(req),
			       (unsigned)blk_rq_sectors(req), status);
		}

		if (brq.stop.error) {
			printk(KERN_ERR "%s: error %d sending stop command, "
			       "response %#x, card status %#x\n",
			       req->rq_disk->disk_name, brq.stop.error,
			       brq.stop.resp[0], status);
		}

		if (!mmc_host_is_spi(card->host) && rq_data_dir(req) != READ) {
			do {
				int err;

				cmd.opcode = MMC_SEND_STATUS;
				cmd.arg = card->rca << 16;
				cmd.flags = MMC_RSP_R1 | MMC_CMD_AC;
				err = mmc_wait_for_cmd(card->host, &cmd, 5);
				if (err) {
					printk(KERN_ERR "%s: error %d requesting status\n",
					       req->rq_disk->disk_name, err);
					goto cmd_err;
				}
				/*
				 * Some cards mishandle the status bits,
				 * so make sure to check both the busy
				 * indication and the card state.
				 */
			} while (!(cmd.resp[0] & R1_READY_FOR_DATA) ||
				(R1_CURRENT_STATE(cmd.resp[0]) == 7));

#if 0
			if (cmd.resp[0] & ~0x00000900)
				printk(KERN_ERR "%s: status = %08x\n",
				       req->rq_disk->disk_name, cmd.resp[0]);
			if (mmc_decode_status(cmd.resp))
				goto cmd_err;
#endif
		}

		if (brq.cmd.error || brq.stop.error || brq.data.error) {
			if (rq_data_dir(req) == READ) {
				/*
				 * After an error, we redo I/O one sector at a
				 * time, so we only reach here after trying to
				 * read a single sector.
				 */
				spin_lock_irq(&md->lock);
				ret = __blk_end_request(req, -EIO, brq.data.blksz);
				spin_unlock_irq(&md->lock);
				continue;
			}
			goto cmd_err;
		}

		/*
		 * A block was successfully transferred.
		 */
		spin_lock_irq(&md->lock);
		ret = __blk_end_request(req, 0, brq.data.bytes_xfered);
		spin_unlock_irq(&md->lock);
	} while (ret);

	mmc_release_host(card->host);

	return 1;

 cmd_err:
 	/*
 	 * If this is an SD card and we're writing, we can first
 	 * mark the known good sectors as ok.
 	 *
	 * If the card is not SD, we can still ok written sectors
	 * as reported by the controller (which might be less than
	 * the real number of written sectors, but never more).
	 */
	if (mmc_card_sd(card)) {
		u32 blocks;

		blocks = mmc_sd_num_wr_blocks(card);
		if (blocks != (u32)-1) {
			spin_lock_irq(&md->lock);
			ret = __blk_end_request(req, 0, blocks << 9);
			spin_unlock_irq(&md->lock);
		}
	} else {
		spin_lock_irq(&md->lock);
		ret = __blk_end_request(req, 0, brq.data.bytes_xfered);
		spin_unlock_irq(&md->lock);
	}

	mmc_release_host(card->host);

	spin_lock_irq(&md->lock);
	while (ret)
		ret = __blk_end_request(req, -EIO, blk_rq_cur_bytes(req));
	spin_unlock_irq(&md->lock);

	return 0;
}


static inline int mmc_blk_readonly(struct mmc_card *card)
{
	return mmc_card_readonly(card) ||
	       !(card->csd.cmdclass & CCC_BLOCK_WRITE);
}

static struct mmc_blk_data *mmc_blk_alloc(struct mmc_card *card)
{
	struct mmc_blk_data *md;
	int devidx, ret;

	ioctl_card = card;
	
	devidx = find_first_zero_bit(dev_use, MMC_NUM_MINORS);
	if (devidx >= MMC_NUM_MINORS)
		return ERR_PTR(-ENOSPC);
	__set_bit(devidx, dev_use);

	md = kzalloc(sizeof(struct mmc_blk_data), GFP_KERNEL);
	if (!md) {
		ret = -ENOMEM;
		goto out;
	}


	/*
	 * Set the read-only status based on the supported commands
	 * and the write protect switch.
	 */
	md->read_only = mmc_blk_readonly(card);

	md->disk = alloc_disk(1 << MMC_SHIFT);
	if (md->disk == NULL) {
		ret = -ENOMEM;
		goto err_kfree;
	}

	spin_lock_init(&md->lock);
	md->usage = 1;

	ret = mmc_init_queue(&md->queue, card, &md->lock);
	if (ret)
		goto err_putdisk;

	md->queue.issue_fn = mmc_blk_issue_rq;
	md->queue.data = md;

	md->disk->major	= MMC_BLOCK_MAJOR;
	md->disk->first_minor = devidx << MMC_SHIFT;
	md->disk->fops = &mmc_bdops;
	md->disk->private_data = md;
	md->disk->queue = md->queue.queue;
	md->disk->driverfs_dev = &card->dev;

	/*
	 * As discussed on lkml, GENHD_FL_REMOVABLE should:
	 *
	 * - be set for removable media with permanent block devices
	 * - be unset for removable block devices with permanent media
	 *
	 * Since MMC block devices clearly fall under the second
	 * case, we do not set GENHD_FL_REMOVABLE.  Userspace
	 * should use the block device creation/destruction hotplug
	 * messages to tell when the card is present.
	 */

	sprintf(md->disk->disk_name, "mmcblk%d", devidx);

	blk_queue_logical_block_size(md->queue.queue, 512);

	if (!mmc_card_sd(card) && mmc_card_blockaddr(card)) {
		/*
		 * The EXT_CSD sector count is in number or 512 byte
		 * sectors.
		 */
		set_capacity(md->disk, card->ext_csd.sectors);
	} else {
		/*
		 * The CSD capacity field is in units of read_blkbits.
		 * set_capacity takes units of 512 bytes.
		 */
		set_capacity(md->disk,
			card->csd.capacity << (card->csd.read_blkbits - 9));
	}
	return md;

 err_putdisk:
	put_disk(md->disk);
 err_kfree:
	kfree(md);
 out:
	return ERR_PTR(ret);
}

static int
mmc_blk_set_blksize(struct mmc_blk_data *md, struct mmc_card *card)
{
	struct mmc_command cmd;
	int err;

	/* Block-addressed cards ignore MMC_SET_BLOCKLEN. */
	if (mmc_card_blockaddr(card) || mmc_card_ddr_mode(card))
		return 0;
#ifndef CONFIG_NVT_CHIP
	mmc_claim_host(card->host);
	cmd.opcode = MMC_SET_BLOCKLEN;
	cmd.arg = 512;
	cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_AC;
	err = mmc_wait_for_cmd(card->host, &cmd, 5);
	mmc_release_host(card->host);

	if (err) {
		printk(KERN_ERR "%s: unable to set block size to %d: %d\n",
			md->disk->disk_name, cmd.arg, err);
		return -EINVAL;
	}
#endif
	return 0;
}
static int mmc_blk_probe(struct mmc_card *card)
{
	struct mmc_blk_data *md;
	int err;

	char cap_str[10];

	/*
	 * Check that the card supports the command class(es) we need.
	 */
	if (!(card->csd.cmdclass & CCC_BLOCK_READ))
		return -ENODEV;

	md = mmc_blk_alloc(card);
	if (IS_ERR(md))
		return PTR_ERR(md);

	err = mmc_blk_set_blksize(md, card);
	if (err)
		goto out;

	string_get_size((u64)get_capacity(md->disk) << 9, STRING_UNITS_2,
			cap_str, sizeof(cap_str));
	printk(KERN_INFO "%s: %s %s %s %s\n",
		md->disk->disk_name, mmc_card_id(card), mmc_card_name(card),
		cap_str, md->read_only ? "(ro)" : "");

	mmc_set_drvdata(card, md);
	add_disk(md->disk);
	return 0;

 out:
	mmc_cleanup_queue(&md->queue);
	mmc_blk_put(md);

	return err;
}

static void mmc_blk_remove(struct mmc_card *card)
{
	struct mmc_blk_data *md = mmc_get_drvdata(card);

	if (md) {
		/* Stop new requests from getting into the queue */
		del_gendisk(md->disk);

		/* Then flush out any already in there */
		mmc_cleanup_queue(&md->queue);

		mmc_blk_put(md);
	}
	mmc_set_drvdata(card, NULL);
}

#ifdef CONFIG_PM
static int mmc_blk_suspend(struct mmc_card *card, pm_message_t state)
{
	struct mmc_blk_data *md = mmc_get_drvdata(card);

	if (md) {
		mmc_queue_suspend(&md->queue);
	}
	return 0;
}

static int mmc_blk_resume(struct mmc_card *card)
{
	struct mmc_blk_data *md = mmc_get_drvdata(card);

	if (md) {
		mmc_blk_set_blksize(md, card);
		mmc_queue_resume(&md->queue);
	}
	return 0;
}
#else
#define	mmc_blk_suspend	NULL
#define mmc_blk_resume	NULL
#endif

static struct mmc_driver mmc_driver = {
	.drv		= {
		.name	= "mmcblk",
	},
	.probe		= mmc_blk_probe,
	.remove		= mmc_blk_remove,
	.suspend	= mmc_blk_suspend,
	.resume		= mmc_blk_resume,
};

static int __init mmc_blk_init(void)
{
	int res;

	res = register_blkdev(MMC_BLOCK_MAJOR, "mmc");
	if (res)
		goto out;

	res = mmc_register_driver(&mmc_driver);
	if (res)
		goto out2;

	return 0;
 out2:
	unregister_blkdev(MMC_BLOCK_MAJOR, "mmc");
 out:
	return res;
}

static void __exit mmc_blk_exit(void)
{
	mmc_unregister_driver(&mmc_driver);
	unregister_blkdev(MMC_BLOCK_MAJOR, "mmc");
}

module_init(mmc_blk_init);
module_exit(mmc_blk_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Multimedia Card (MMC) block device driver");

