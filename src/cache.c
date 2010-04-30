/**
 * Implements the buffer cache used to reduce the number of disk accesses. The
 * cache will also hopefully aid in other optimisations such as copy-on-write
 * and block remapping.
 *
 */

#define _GNU_SOURCE		/* for O_DIRECT */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "const.h"
#include "comms.h"
#include "short_array.h"
#include "cache.h"

struct minix_block *front;	/* front of buffer chain. LRU. */
struct minix_block *rear;	/* back of buffer chain. MRU. */
struct minix_block 
	*cache_hash[NR_BUF_HASH];	/* hash table of block chains */
int bufs_in_use = 0;			/* number of buffers in use */
int fd;					/* I/O device file descriptor */
struct short_array *write_log;		/* where we record every block written
					 * to disk. */

unsigned long cache_read_count = 0;	/* number of device reads */

/**
 * Create a new empty block with the data portion set to BLOCK_SIZE bytes
 * and correctly aligned for O_DIRECT I/O.
 */
static struct minix_block *mk_block(void)
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

/**
 * Writes the given block to the device previously opened by
 * open_blk_device(...).
 *
 * Failure to write the block will result in an error message and the program
 * terminating.
 */
static void write_block(struct minix_block *blk)
{
	int disk_offset = blk->blk_nr * BLOCK_SIZE;
	info("\033[31mwrite_block(%d): writing block %d to disk offset %d...\033[0m", 
		blk->blk_nr, blk->blk_nr, disk_offset);
	
	short_array_add(blk->blk_nr, write_log);	
	
	if(lseek(fd, disk_offset, SEEK_SET) != disk_offset) {
		/* seek failed */
		panic("write_block(%d): unable to seek to disk offset %d", 
			blk->blk_nr, disk_offset);
	}

	if(write(fd, blk->blk_data, BLOCK_SIZE) != BLOCK_SIZE) {
		panic("write_block(%d): unable to write all block data",
			blk->blk_nr);
	}
	
	blk->blk_dirty = FALSE;
}

/**
 * Reads a data block from the device previously opened by open_blk_device(...)
 * and copies it over to the data section of the given minix_block struct. The
 * block read is set in blk->blk_nr.
 *
 * Failure to read the block will result in an error message and the program
 * terminating.
 */ 
static void read_block(struct minix_block *blk)
{
	int disk_offset = blk->blk_nr * BLOCK_SIZE;

	info("\033[32mread_block(%d): reading block %d from disk offset %d...\033[0m", 
		blk->blk_nr, blk->blk_nr, disk_offset);

	cache_read_count++;

	if(lseek(fd, disk_offset, SEEK_SET) != disk_offset) {
		panic("read_block(%d): unable to seek to disk offset %d",
			blk->blk_nr, disk_offset);
	}
	if(read(fd, blk->blk_data, BLOCK_SIZE) != BLOCK_SIZE) {
		panic("read_block(%d): unable to read all block data",
			blk->blk_nr);
	}
	blk->blk_dirty = FALSE;
}

static void print_cache_block(struct minix_block *blk)
{
	printf("  blk_nr: %d\n", blk->blk_nr);
	printf("  blk_dirty: %s\n", blk->blk_dirty == TRUE ? "true" : "false");
	printf("  blk_data: %p\n", blk->blk_data);
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

/**
 * Opens the block device on which the cache will operate.
 *
 * Failure to open the device will result with an error message and program 
 * terminating.
 */
void open_blk_device(const char *d)
{
	/* attempt to open device with flags:
 	 *  O_RDWR 	- for reading and writing
 	 *  O_DIRECT 	- for direct I/O with the device. I.e bypassing block
 	 *  		  buffer cache.
 	 */
	if((fd = open(d, (O_RDWR | O_DIRECT))) < 0) {
		/* problem with opening device */
		panic("open_device(\"%s\"): unable to open device", d);
	}
	debug("open_device(\"%s\"): successfully opened device", d);
}

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

	debug("init_cache(): allocating %d cache blocks...", NR_BUFS);
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

		blk->blk_hash = front;	/* just to set up initial hash chain */
		front = blk;		/* finially place new block at front */
	}
	
	debug("init_cache(): clearing cache hash table of size %d...", 
		NR_BUF_HASH);
	for(i = 0; i < NR_BUF_HASH; i++)
		cache_hash[i] = NIL_BUF;

	debug("init_cache(): creating initial cache block hash chain at "
		"cache_hash[%d]...",
		 NO_BLOCK & (NR_BUF_HASH - 1));
	cache_hash[NO_BLOCK & (NR_BUF_HASH - 1)] = front;
	
	/* init the write log */
	write_log = short_array_init(ARR_DEFAULT_SIZE);	
}

/**
 * Clean up the cache and write the log file.
 *
 * After a cache destory, an init_cache must be used to start using the cache
 * again.
 */
void cache_destroy(void)
{
	/* TODO: Should really clean up cache memory. */

	info_1("cache_destory(): total device reads = %u\n", cache_read_count);

	info_1("cache_destroy(): saving %d entries from cache write log to"
		" %s... ", write_log->size, CACHE_WRITE_LOG_FILE);

	if(short_array_unsigned_write(write_log, CACHE_WRITE_LOG_FILE) == 0) {
		info_1("cache_destroy(): write successful");
	}
	else {
		info_1("cache_destroy(): write failed");
	}

	debug("cache_destroy(): destroying cache write log...");
	short_array_destroy(write_log);
}

/**
 * Sync's the buffer cache by writing all dirty blocks to disk.
 *
 * Returns the number of dirty blocks written.
 */
int sync_cache(void)
{
	struct minix_block *blk;
	int flush_count = 0;
	blk = front;
	while(blk != NIL_BUF) {
		//if(blk->blk_dirty == TRUE && blk->blk_count == 0) {
		if(blk->blk_dirty == TRUE) {
			/* found block that is dirty and not in use so flush it
			 * to disk. */
			debug("sync_cache(): flushing block %d...", 
				blk->blk_nr);
			write_block(blk);
			flush_count++;
		}
		blk = blk->blk_next;
	}

	return flush_count;
}

/**
 * Returns a minix_block struct corresponding to block identified by blk_nr. 
 * The buffer cache is first searched. If the block is found in the cache its
 * usage count is incremented and a pointer is returned. 
 *
 * If the block is not found in the cache then we attempt to evict an item as 
 * close toward the front of the cache (the LRU end) as possible and then use 
 * the evicted cache bufferto hold the requested block. The requested block is
 * only read from disk if do_read is set to TRUE. It is not always necessary to
 * read a block from disk if the calling code knows in advance that it is
 * going to overwrite all data in the block. In this instance do_read is set to
 * FALSE and a free block is returned without any disk I/O being made.
 */
struct minix_block *get_block(int blk_nr, char do_read)
{

	register struct minix_block *blk, *prev_ptr;

	blk = cache_hash[blk_nr & (NR_BUF_HASH - 1)];
	
	/* try to find the block requested in the cache */
	while(blk != NIL_BUF) {
		if(blk->blk_nr == blk_nr) {
			/* cache hit */
			if(blk->blk_count == 0) bufs_in_use++;
			blk->blk_count++; /* block is now in use */
			debug("get_block(%d): cache hit", blk_nr);
			return blk;
		}
		
		/* not the block we want, get next one in the hash chain */
		blk = blk->blk_hash;
	}

	/* cache miss. we'll have to find a free space in our cache and read 
	 * the block in to that space. a free space is a space not in use 
	 * (i.e blk_count == 0)*/
	debug("get_block(%d): cache miss", blk_nr);
	if(bufs_in_use == NR_BUFS)
		panic("get_block(...): cannot read in block from disk. all "
			"buffers are in use");
	bufs_in_use++;
	blk = front;
	while(blk->blk_count > 0 && blk->blk_next != NIL_BUF) 
		blk = blk->blk_next;
	if(blk->blk_count > 0) {
		/* we looked through entire chain and couldn't find a free
 		 * space. */
		panic("get_block(...): cannot read in block from disk. "
			"searched all buffers and all are in use");
	}

	debug("get_block(%d): evicting block %d from cache...", blk_nr, 
		blk->blk_nr);	
	/* blk is now the block we will fill with data from disk.
 	 * remove the block from its existing hash chain */
	prev_ptr = cache_hash[blk->blk_nr & (NR_BUF_HASH - 1)];
	if(prev_ptr == blk) {
		/* the block was at the front of the hash chain */
		cache_hash[blk->blk_nr & (NR_BUF_HASH - 1)] = blk->blk_hash;
	}
	else {
		/* the block is not at the front of the chain so we're gonna
 		 * have to search the hash chain to find its position. */
		while(prev_ptr->blk_hash != NIL_BUF) {
			if(prev_ptr->blk_hash == blk) {
				/* found it */
				prev_ptr->blk_hash = blk->blk_hash;
				break;
			}
			
			/* keep on looking */
			prev_ptr = prev_ptr->blk_hash;
		}
	}
	
	/* dirty blocks must be written to disk */
	if(blk->blk_dirty == TRUE) write_block(blk);

	/* fill in block fields and add to the hash chain corresponding to the
	 * new block number */
	blk->blk_nr = blk_nr;
	blk->blk_count++;
	blk->blk_hash = cache_hash[blk_nr & (NR_BUF_HASH - 1)];
	cache_hash[blk_nr & (NR_BUF_HASH - 1)] = blk;

	/* read the block in from disk if necessary. it won't always be
	 * necessary if the routine calling get_block expects to re-write the 
	 * entire block anyway. */
	if(do_read == TRUE) read_block(blk);

	return blk;
}

void put_block(struct minix_block *blk, int block_type)
{
	register struct minix_block *next_ptr, *prev_ptr;

	if(blk == NIL_BUF) { 
		debug("put_block(?): attempting to put a NIL block");
		return;
	}

	if(blk->blk_count == 0) {
		panic("put_block(%d): attempting to put block that is not in "
			"use", blk->blk_nr);
	}

	blk->blk_count--;
	if(blk->blk_count > 0) {
		/* block still in use */
		debug("put_block(%d, %d): block still in use. not modifying "
			"LRU chain", blk->blk_nr, block_type);
		return;
	}
	
	bufs_in_use--;
	next_ptr = blk->blk_next;
	prev_ptr = blk->blk_prev;
	
	/* remove this block from current position in LRU chain */
	if(prev_ptr == NIL_BUF) {
		/* block is on front of LRU chain */
		front = next_ptr;
	}
	else {
		prev_ptr->blk_next = next_ptr;
	}
	if(next_ptr == NIL_BUF) {
		/* block is on the back of LRU chain */
		rear = prev_ptr;
	}
	else {
		next_ptr->blk_prev = prev_ptr;
	}

	/* now put the block back on the LRU chain. the position we place the
	 * block at is determined by the block_type. some blocks when 'put' are
	 * likely to be requested again (e.g inode or directory blocks) whereas
	 * others are less likely to be requested again (bitmaps and 
	 * superblocks since they are stored in memory).
	 */
	if(block_type & ONE_SHOT) {
		/* block probably won't be needed again soon so put it on the
		 * front of the LRU chain. */
		debug("put_block(%d, %d): putting block on front of LRU chain",
			blk->blk_nr, block_type);
		blk->blk_prev = NIL_BUF;	/* nothing goes before front */
		blk->blk_next = front;
		if(front == NIL_BUF)
			rear = blk;		/* LRU chain was empty */
		else
			front->blk_prev = blk;
		front = blk;
	}
	else {
		/* block is more likely to be requested again soon so place it
		 * at the back of the LRU chain. */
		debug("put_block(%d, %d): putting block on rear of LRU chain",
			blk->blk_nr, block_type);
		blk->blk_prev = rear;
		blk->blk_next = NIL_BUF;	/* nothing goes after rear */
		if(rear == NIL_BUF)
			front = blk;		/* LRU chain was empty */
		else
			rear->blk_next = blk;
		rear = blk;
	}

	/* write the critical blocks to disk immedietely instead of waiting for
	 * them to be written by a sync or when otherwise emptied from the 
	 * buffer cache. */
	if((block_type & WRITE_IMMED) && blk->blk_dirty == TRUE) {
		debug("put_block(%d, %d): critical block is dirty, writing "
			"immedietely...", blk->blk_nr, block_type);
		write_block(blk);
	}

}


	

	




