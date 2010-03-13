/**
 * Implements the buffer cache used to reduce the number of disk accesses. The
 * cache will also hopefully aid in other optimisations such as copy-on-write
 * and block remapping.
 *
 */

/**
 * Represents a block of data present in our buffer cache.
 */
struct minix_block {
	/* header portion of block */
	int blk_nr;				/* the block number */
	struct minix_block *blk_next;		/* next block in chain */
	struct minix_block *blk_prev;		/* prev block in chain */
	struct minix_block *blk_hash;		/* next block in hash chain */
	char blk_dirty;				/* clean or dirty */
	char blk_count;				/* # of users of this block */

	/* data portion of block */
	char *blk_data;			/* should be aligned for O_DIRECT */
};

#define NIL_BUF (struct minix_block *)0		/* no buffer entry */

extern struct minix_block 
	*cache_hash[NR_BUF_HASH];		/* buffer hash table */	
extern struct minix_block *front;
extern struct minix_block *rear;	
extern int block_in_use;

/**
 * Initiate the buffer cache by preallocating the necessary buffers and linking
 * them together.
 */
void init_cache(void);

/**
 * Create a new empty block with the data portion set to BLOCK_SIZE bytes and
 * correctly aligned for O_DIRECT I/O.
 */
struct minix_block *mk_block(void);

void print_cache(void);
void print_cache_block(struct minix_block *blk);
