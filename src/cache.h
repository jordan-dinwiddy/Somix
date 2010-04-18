#ifndef _MINIX_CACHE
#define _MINIX_CACHE

#define CACHE_WRITE_LOG_FILE "cache_write.log"

/* Define if we do _not_ want to immedietely flush critical blocks such as
 * inodes and indirect mapping blocks to storage. */
#define CACHE_WRITE_IMMED_OFF

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

#define WRITE_IMMED 	0100	/* write block straight away */
#define ONE_SHOT 	0200	/* block unlikely to be needed soon */

#ifndef CACHE_WRITE_IMMED_OFF
#define INODE_BLOCK	0 + WRITE_IMMED			/* inode block */
#define DIR_BLOCK	1 + WRITE_IMMED			/* directory block */
#define INDIRECT_BLOCK	2 + WRITE_IMMED			/* pointer block */
#define IMAP_BLOCK	3 + WRITE_IMMED + ONE_SHOT	/* inode bitmap */
#define ZMAP_BLOCK	4 + WRITE_IMMED + ONE_SHOT	/* zone_bitmap */
#define SUPER_BLOCK	5 + WRITE_IMMED + ONE_SHOT	/* superblock */
#define DATA_BLOCK	6				/* normal data block */

#else

#define INODE_BLOCK	0 			/* inode block */
#define DIR_BLOCK	1 			/* directory block */
#define INDIRECT_BLOCK	2 			/* pointer block */
#define IMAP_BLOCK	3 + ONE_SHOT		/* inode bitmap */
#define ZMAP_BLOCK	4 + ONE_SHOT		/* zone_bitmap */
#define SUPER_BLOCK	5 + ONE_SHOT		/* superblock */
#define DATA_BLOCK	6			/* normal data block */
#endif

/**
 * Opens the block device on which the cache will operate.
 *
 * Failure to open the device will result with an error message and the program
 * terminating.
 */
void open_blk_device(const char *d);

/**
 * Initiate the buffer cache by preallocating the necessary buffers and linking
 * them together.
 */
void init_cache(void);


struct minix_block *get_block(int blk_nr, char do_read);
void put_block(struct minix_block *blk, int block_type);

/* disk I/O */
int sync_cache(void);

void print_cache(void);

void cache_destroy(void);
#endif
