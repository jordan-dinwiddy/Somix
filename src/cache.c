/**
 * Implements the buffer cache used to reduce the number of disk accesses. The
 * cache will also hopefully aid in other optimisations such as copy-on-write
 * and block remapping.
 *
 */

#include <stdlib.h>
#include "const.h"
#include "comms.h"
#include "cache.h"
#include <stdio.h>

struct minix_block *front;	/* front of buffer chain. LRU. */
struct minix_block *rear;	/* back of buffer chain. MRU. */

/**
 * Initiate the buffer cache by preallocating the necessary buffers and
 * linking them together.
 */
void init_cache(void)
{
	int i;
	struct minix_block *blk;
	
	front = NIL_BUF;
	rear = NIL_BUF;

	debug("init_cache(): allocating cache blocks...");
	for(i = 0; i < NR_BUFS; i++) {
		blk = mk_block();
		
		/* put block on front of chain */
		if(front == NIL_BUF) {
			rear = blk;
		}
		else { 
			front->blk_prev = blk;
			blk->blk_next = front;
		}
		front = blk;
	}
	debug("init_cache(): all cache blocks allocated");
							
}

void print_cache(void)
{
	struct minix_block *blk;
	blk = front;
	int i = 0;

	while(blk != NIL_BUF) {
		printf("Cache Block %d:\n", i++);
		print_cache_block(blk);
		blk = blk->blk_next;
	}
}

void print_cache_block(struct minix_block *blk)
{
	printf("  blk_nr: %d\n", blk->blk_nr);
	printf("  blk_dirty: %s\n", blk->blk_dirty == TRUE ? "true" : "false");
	printf("  blk_data: %p\n", blk->blk_data);
}


/**
 * Create a new empty block with the data portion set to BLOCK_SIZE bytes
 * and correctly aligned for O_DIRECT I/O.
 */
struct minix_block *mk_block(void)
{
	struct minix_block *blk = (struct minix_block *) 
		malloc(sizeof(struct minix_block));
	if(blk == NULL)
		panic("mk_block(): unable to malloc new block");

	blk->blk_nr = NO_BLOCK;
	blk->blk_next = NIL_BUF;
	blk->blk_prev = NIL_BUF;
	blk->blk_hash = NIL_BUF;
	blk->blk_dirty = FALSE;
	blk->blk_count = 0;

	/* allocate BLOCK_SIZE bytes for data region of our block. this region
 	 * must be mem aligned when using direct I/O (via the O_DIRECT flag)
 	 * with our device. we do this to bypass the linux buffer cache. */
	if(posix_memalign((void **) &blk->blk_data, BLOCK_ALIGN, 
		BLOCK_SIZE) != 0) 
		panic("mk_block(): unable to posix_memalign data region of "
			"new block");

	/* everything constructed okay, return our new block */
	return blk;
}

	

	




