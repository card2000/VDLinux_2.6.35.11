/*
 * Squashfs - a compressed read only filesystem for Linux
 *
 * Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008
 * Phillip Lougher <phillip@lougher.demon.co.uk>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * block.c
 */

/*
 * This file implements the low-level routines to read and decompress
 * datablocks and metadata blocks.
 */

#include <linux/fs.h>
#include <linux/vfs.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/buffer_head.h>

#include "squashfs_fs.h"
#include "squashfs_fs_sb.h"
#include "squashfs_fs_i.h"
#include "squashfs.h"
#include "decompressor.h"

/*
 * Read the metadata block length, this is stored in the first two
 * bytes of the metadata block.
 */
static struct buffer_head *get_block_length(struct super_block *sb,
			u64 *cur_index, int *offset, int *length)
{
	struct squashfs_sb_info *msblk = sb->s_fs_info;
	struct buffer_head *bh;

	bh = sb_bread(sb, *cur_index);
	if (bh == NULL)
		return NULL;

	if (msblk->devblksize - *offset == 1) {
		*length = (unsigned char) bh->b_data[*offset];
		put_bh(bh);
		bh = sb_bread(sb, ++(*cur_index));
		if (bh == NULL)
			return NULL;
		*length |= (unsigned char) bh->b_data[0] << 8;
		*offset = 1;
	} else {
		*length = (unsigned char) bh->b_data[*offset] |
			(unsigned char) bh->b_data[*offset + 1] << 8;
		*offset += 2;
	}

	return bh;
}

#ifdef CONFIG_SQUASHFS_DEBUGGER

extern char last_file_name[100];

#define	DATABLOCK	0
#define	METABLOCK	1
#define	VERSION_INFO	"Ver 1.1"

u64	__cur_index;
int	__offset;
int	__length;

/*
 * Dump out the contents of some memory nicely...
 */
static void dump_mem_be(const char *lvl, const char *str, unsigned long bottom,
		unsigned long top)
{
	unsigned long first;
	mm_segment_t fs;
	int i;

	/*
	 * We need to switch to kernel mode so that we can use __get_user
	 * to safely read from kernel space.  Note that we now dump the
	 * code first, just in case the backtrace kills us.
	 */
	fs = get_fs();
	set_fs(KERNEL_DS);

	printk("%s%s(0x%08lx to 0x%08lx)\n", lvl, str, bottom, top);

	for (first = bottom & ~31; first < top; first += 32) {
		unsigned long p;
		char str[sizeof(" 12345678") * 8 + 1];

		memset(str, ' ', sizeof(str));
		str[sizeof(str) - 1] = '\0';

		for (p = first, i = 0; i < 8 && p <= top; i++, p += 4) {
			if (p >= bottom && p <= top) {
				unsigned long val;
				if (__get_user(val, (unsigned long *)p) == 0)
				{
					val = __cpu_to_be32(val);
					sprintf(str + i * 9, " %08lx", val);
				}
				else
					sprintf(str + i * 9, " ????????");
			}
		}
		printk("%s%04lx:%s\n", lvl, first & 0xffff, str);
	}

	set_fs(fs);
}

int sp_memcmp(const void *cs, const void *ct, size_t count, int* offset)
{
	const unsigned char *su1, *su2;
	int res = 0;

	printk("memcmp: 0x%p-0x%p 0x%08x-0x%08x %04d \n",
			cs,ct,*(unsigned int*)cs,*(unsigned int*)ct,count);

	*offset = count;

	for (su1 = cs, su2 = ct; 0 < count; ++su1, ++su2, count--)
		if ((res = *su1 - *su2) != 0)
			break;

	*offset = *offset - count;

	return res;
}

#endif

/*
 * Read and decompress a metadata block or datablock.  Length is non-zero
 * if a datablock is being read (the size is stored elsewhere in the
 * filesystem), otherwise the length is obtained from the first two bytes of
 * the metadata block.  A bit in the length field indicates if the block
 * is stored uncompressed in the filesystem (usually because compression
 * generated a larger block - this does occasionally happen with zlib).
 */
int squashfs_read_data(struct super_block *sb, void **buffer, u64 index,
			int length, u64 *next_index, int srclength, int pages)
{
	struct squashfs_sb_info *msblk = sb->s_fs_info;
	struct buffer_head **bh;
	int offset = index & ((1 << msblk->devblksize_log2) - 1);
	u64 cur_index = index >> msblk->devblksize_log2;
	int bytes, compressed = 0, b = 0, k = 0, page = 0, avail;

#ifdef CONFIG_SQUASHFS_DEBUGGER
	char print_buffer[120] = {0, };
	int block_type;     /* 0: DataBlock, 1: Metadata block */
	int all_block_read; /* 0: Read partial block , 1: Read all compressed block */
	int fail_block;
	int i;
	int old_k = 0;
	char bdev_name[BDEVNAME_SIZE];

	__cur_index = cur_index;
	__offset    = offset;
	__length    = length;
#endif

	bh = kcalloc(((srclength + msblk->devblksize - 1)
		>> msblk->devblksize_log2) + 1, sizeof(*bh), GFP_KERNEL);
	if (bh == NULL)
		return -ENOMEM;

	if (length) {
		/*
		 * Datablock.
		 */
#ifdef CONFIG_SQUASHFS_DEBUGGER
		block_type = DATABLOCK;
#endif
		bytes = -offset;
		compressed = SQUASHFS_COMPRESSED_BLOCK(length);
		length = SQUASHFS_COMPRESSED_SIZE_BLOCK(length);
		if (next_index)
			*next_index = index + length;

		TRACE("Block @ 0x%llx, %scompressed size %d, src size %d\n",
			index, compressed ? "" : "un", length, srclength);

		if (length < 0 || length > srclength ||
				(index + length) > msblk->bytes_used)
			goto read_failure;

		for (b = 0; bytes < length; b++, cur_index++) {
			bh[b] = sb_getblk(sb, cur_index);
			if (bh[b] == NULL)
				goto block_release;
			bytes += msblk->devblksize;
		}
		ll_rw_block(READ, b, bh);

	} else {
		/*
		 * Metadata block.
		 */
#ifdef CONFIG_SQUASHFS_DEBUGGER
		block_type = METABLOCK;
#endif
		if ((index + 2) > msblk->bytes_used)
			goto read_failure;

		bh[0] = get_block_length(sb, &cur_index, &offset, &length);
		if (bh[0] == NULL)
			goto read_failure;
		b = 1;

		bytes = msblk->devblksize - offset;
		compressed = SQUASHFS_COMPRESSED(length);
		length = SQUASHFS_COMPRESSED_SIZE(length);
		if (next_index)
			*next_index = index + length + 2;

		TRACE("Block @ 0x%llx, %scompressed size %d\n", index,
				compressed ? "" : "un", length);

		if (length < 0 || length > srclength ||
					(index + length) > msblk->bytes_used)
			goto block_release;

		for (; bytes < length; b++) {
			bh[b] = sb_getblk(sb, ++cur_index);
			if (bh[b] == NULL)
				goto block_release;
			bytes += msblk->devblksize;
		}
		ll_rw_block(READ, b - 1, bh + 1);

	}

	if (compressed) {
		length = squashfs_decompress(msblk, buffer, bh, b, offset,
			 length, srclength, pages);
		if (length < 0)
			goto read_failure;
	} else {
		/*
		 * Block is uncompressed.
		 */
		int i, in, pg_offset = 0;

		for (i = 0; i < b; i++) {
			wait_on_buffer(bh[i]);
			if (!buffer_uptodate(bh[i]))
				goto block_release;
		}

		for (bytes = length; k < b; k++) {
			in = min(bytes, msblk->devblksize - offset);
			bytes -= in;
			while (in) {
				if (pg_offset == PAGE_CACHE_SIZE) {
					page++;
					pg_offset = 0;
				}
				avail = min_t(int, in, PAGE_CACHE_SIZE -
						pg_offset);
				memcpy(buffer[page] + pg_offset,
						bh[k]->b_data + offset, avail);
				in -= avail;
				pg_offset += avail;
				offset += avail;
			}
			offset = 0;
			put_bh(bh[k]);
		}
	}

	kfree(bh);
	return length;

block_release:
#ifdef CONFIG_SQUASHFS_DEBUGGER
	old_k = k;
#endif // CONFIG_SQUASHFS_DEBUGGER
	for (; k < b; k++)
		put_bh(bh[k]);

read_failure:

#ifdef CONFIG_SQUASHFS_DEBUGGER
	if( old_k < b - 1 )     /* Failed during inflate *partial* block, so last read-block is the reason of failure */
	{
		fail_block = old_k;
		all_block_read = 0;
	}
	else                /* Failed during inflate *all* block, so We can't know what is fail_block */
	{
		fail_block = 0;
		all_block_read = 1;
	}

	preempt_disable();
	printk(KERN_ERR "======================================================= %7s =====\n",VERSION_INFO);
	if(block_type == METABLOCK)         /* METABLOCK */
	{
		printk(KERN_ERR"[MetaData Block]\nBlock @ 0x%llx, %scompressed size %d, nr of b: %d\n", index,
				compressed ? "" : "un", length, b - 1);
		if(old_k == 0)
			/* get_block_length() failure*/
			printk(KERN_ERR "- Metablock 0 is broken.. compressed block - bh[0]\n");
		else
			printk(KERN_ERR "- %s all compressed block bh[0] + (%d/%d)\n",
					all_block_read ? "Read":"Didn't read",
					all_block_read ? b-1 : old_k, b-1);
	}
	else                                /* DATABLOCK */
	{
		printk(KERN_ERR"[DataBlock]\nBlock @ 0x%llx, %scompressed size %d, src size %d, nr of bh :%d\n",
				index, compressed ? "" : "un", length, srclength, b);
		printk(KERN_ERR "- %s all compressed block (%d/%d)\n",
				all_block_read ? "Read":"Didn't read",
				all_block_read ? b : old_k+1, b);
	}

	printk(KERN_ERR "---------------------------------------------------------------------\n");
	if( next_index )
	{
		printk(KERN_ERR "[Block: 0x%08llx(0x%08llx) ~ 0x%08llx(0x%08llx)]\n",
				index >> msblk->devblksize_log2, index,
				*next_index >> msblk->devblksize_log2, *next_index);
	}
	else
	{
		printk(KERN_ERR "[Block: 0x%08llx(0x%08llx) ~ UNDEFINED]\n",
				index >> msblk->devblksize_log2, index);
	}
	printk(KERN_ERR "\tlength:%d , device block_size : %d \n",
			length, msblk->devblksize /*msblk->block_size*/);
	printk(KERN_ERR "---------------------------------------------------------------------\n");

	if(all_block_read)  /* First Block info */
		printk(KERN_ERR "<< First Block Info >>\n");
	else                /* Fail Block info */
		printk(KERN_ERR "<< Fail Block Info >>\n");

	printk(KERN_ERR "- bh->b_blocknr : %4llu (0x%08llx x %4d byte = 0x%08llx)\n",
			bh[fail_block]->b_blocknr,
			bh[fail_block]->b_blocknr,
			bh[fail_block]->b_size,
			bh[fail_block]->b_blocknr * bh[fail_block]->b_size);
	printk(KERN_ERR "- bi_sector     : %4llu (0x%08llx x  512 byte = 0x%08llx)\n",
			bh[fail_block]->b_blocknr * (bh[fail_block]->b_size >> 9),
			bh[fail_block]->b_blocknr * (bh[fail_block]->b_size >> 9),
			bh[fail_block]->b_blocknr * (bh[fail_block]->b_size >> 9) * 512 /* sector size = 512byte fixed */);
	printk(KERN_ERR "bh[%d]->b_data:%p\n", fail_block, bh[fail_block]->b_data);
	printk(KERN_ERR "---------------------------------------------------------------------\n");
	printk(KERN_ERR "Device : %s \n", bdevname(sb->s_bdev, bdev_name));



	if(block_type == METABLOCK) /* MetaData Block */
	{
		printk(KERN_ERR "MetaData Access Error : Maybe mount or ls problem..????\n");
	}
	else						/* Data Block */
	{
		printk(KERN_ERR "LAST ACCESS FILE : %s\n",last_file_name);
	}

	printk(KERN_ERR "---------------------------------------------------------------------\n");

	for(i = 0 ; i < b; i++)
	{
		printk(KERN_ERR "bh[%2d]:0x%p",i,bh[i]);
		if(bh[i])
		{
			printk(" | bh[%2d]->b_data:0x%p | ",i,bh[i]->b_data);
			printk("bh value :0x%08x",
					__cpu_to_be32(*(unsigned int*)(bh[i]->b_data)));
			if(fail_block == i)
				printk("*");
		}
		printk("\n");
	}
	printk(KERN_ERR "---------------------------------------------------------------------\n");

	for(i = 0 ; i < b; i++)
	{
		printk(KERN_ERR "bh[%2d]:0x%p",i,bh[i]);
		if(bh[i])
		{
			printk(" | bh[%2d]->b_data:0x%p | \n",i,bh[i]->b_data);
			dump_mem_be(KERN_ERR, "DUMP BH->b_data",
					(unsigned int)bh[i]->b_data,
					(unsigned int)bh[i]->b_data + msblk->devblksize - 4);

		}
		printk("\n");
	}

	preempt_enable();

	/* == verifying flash routine == */
	printk(KERN_ERR "---------------------------------------------------------------------\n");
	printk(KERN_ERR "[verifying flash data]\n");

	if (1)
	{
		char* block_buffer = NULL;
		int diff_offset = 0;
		int found_block = 0;
		int check_unit = 0;

		if( all_block_read )
			check_unit = b;
		else
			check_unit = 1;

		/* 1. buffer allocation */
		block_buffer = kmalloc(msblk->devblksize * check_unit, GFP_KERNEL);

		if( block_buffer == NULL )
		{
			printk(KERN_EMERG "verifying flash failed - not enough memory \n");
			goto debug_malloc_fail;
		}

		/* 2. copy original data & inv cache */
		for(i = fail_block ; i < fail_block + check_unit; i++)
		{
			flush_cache_all();
			clear_buffer_uptodate(bh[i]);   //force to read again

			if( all_block_read )
				memcpy(block_buffer + i * msblk->devblksize, bh[i]->b_data, msblk->devblksize);
			else
				memcpy(block_buffer, bh[i]->b_data, msblk->devblksize);
		}

		/* 3. Reread buffer block from flash */
		if( block_type == DATABLOCK )   /* Datablock */
		{
			ll_rw_block(READ, b, bh);
		}
		else                            /* Metablock */
		{
			/* 1st block of Metadata is used for special purpose */
			bh[0] = get_block_length(sb, &__cur_index, &__offset, &__length);
			ll_rw_block(READ, b - 1, bh + 1);
		}

		/* 4. buffer comparision */
		for(i = fail_block ; i < fail_block + check_unit; i++)
		{
			if( all_block_read )
				found_block = sp_memcmp(block_buffer + (msblk->devblksize * i), 
										bh[i]->b_data, msblk->devblksize, 
										&diff_offset);
			else
				found_block = sp_memcmp(block_buffer, bh[i]->b_data, msblk->devblksize, &diff_offset);

			if( found_block )
			{
				diff_offset = diff_offset & ~(0x3);
				fail_block = i; /* We found fail block */
				break;
			}
			diff_offset = 0;
		}

		/* 5. print the result */
		if( found_block )
		{
			printk(KERN_ERR "bh[%2d]:0x%p | bh[%2d]->b_data:%p\n",
					fail_block, bh[fail_block], fail_block, bh[fail_block]->b_data);

			printk(KERN_ERR "---------------------------------------------------------------------\n");
			dump_mem_be(KERN_ERR, "DUMP Fail Block ( Orignal data - from fresh flash) ",
					(unsigned int)bh[fail_block]->b_data,
					(unsigned int)bh[fail_block]->b_data + msblk->devblksize - 4); /* e.g. 4k */

			printk(KERN_ERR "---------------------------------------------------------------------\n");
			printk(KERN_ERR "[verifying flash - *Diff* : memory problem..??] \n");
			printk(KERN_ERR "---------------------------------------------------------------------\n");

			printk(KERN_ERR "bh[%2d]: 0x%p | \nbh[%2d]->b_data: 0x%p + 0x%x \n\n",
					fail_block, bh[fail_block], fail_block, bh[fail_block]->b_data, diff_offset);


			if( all_block_read )
				printk(KERN_ERR "0x%x => 0x%x (big endian)\n",
						__cpu_to_be32(*(unsigned int*)(bh[fail_block]->b_data + diff_offset)),
						__cpu_to_be32(*(unsigned int*)(block_buffer + (fail_block * msblk->devblksize) + diff_offset)) );
			else
				printk(KERN_ERR "0x%x => 0x%x (big endian)\n",
						__cpu_to_be32(*(unsigned int*)(bh[fail_block]->b_data + diff_offset)),
						__cpu_to_be32(*(unsigned int*)(block_buffer + diff_offset)) );
		}
		else
		{
			printk(KERN_ERR "---------------------------------------------------------------------\n");
			printk(KERN_ERR "[verifying flash - *Same* : flash problem..??,"
								"but should need compare with image file.!!]\n");
			printk(KERN_ERR "---------------------------------------------------------------------\n");
		}

		kfree(block_buffer);
	}
	printk(KERN_ERR "=====================================================================\n");

debug_malloc_fail:
#endif

	ERROR("squashfs_read_data failed to read block 0x%llx\n",
					(unsigned long long) index);
	kfree(bh);

	return -EIO;
}
