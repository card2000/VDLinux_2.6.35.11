/*
 * Basic general purpose allocator for managing special purpose memory
 * not managed by the regular kmalloc/kfree interface.
 * Uses for this includes on-device special memory, uncached memory
 * etc.
 *
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */


#ifdef CONFIG_CMA
struct gen_pool;

struct gen_pool *__must_check gen_pool_create(unsigned order, int nid);

int __must_check gen_pool_add(struct gen_pool *pool, unsigned long addr,
			      size_t size, int nid);

void gen_pool_destroy(struct gen_pool *pool);

unsigned long __must_check
gen_pool_alloc_aligned(struct gen_pool *pool, size_t size,
		       unsigned alignment_order);

/**
 * gen_pool_alloc() - allocate special memory from the pool
 * @pool:	Pool to allocate from.
 * @size:	Number of bytes to allocate from the pool.
 *
 * Allocate the requested number of bytes from the specified pool.
 * Uses a first-fit algorithm.
 */
static inline unsigned long __must_check
gen_pool_alloc(struct gen_pool *pool, size_t size)
{
	return gen_pool_alloc_aligned(pool, size, 0);
}

void gen_pool_free(struct gen_pool *pool, unsigned long addr, size_t size);

#else
/*
 *  General purpose special memory pool descriptor.
 */
struct gen_pool {
	rwlock_t lock;
	struct list_head chunks;	/* list of chunks in this pool */
	int min_alloc_order;		/* minimum allocation order */
};

/*
 *  General purpose special memory pool chunk descriptor.
 */
struct gen_pool_chunk {
	spinlock_t lock;
	struct list_head next_chunk;	/* next chunk in pool */
	unsigned long start_addr;	/* starting address of memory chunk */
	unsigned long end_addr;		/* ending address of memory chunk */
	unsigned long bits[0];		/* bitmap for allocating memory chunk */
};

extern struct gen_pool *gen_pool_create(int, int);
extern int gen_pool_add(struct gen_pool *, unsigned long, size_t, int);
extern void gen_pool_destroy(struct gen_pool *);
extern unsigned long gen_pool_alloc(struct gen_pool *, size_t);
extern void gen_pool_free(struct gen_pool *, unsigned long, size_t);

#endif


