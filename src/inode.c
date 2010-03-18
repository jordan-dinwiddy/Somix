#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "const.h"
#include "types.h"
#include "comms.h"
#include "cache.h"
#include "superblock.h"
#include "inode.h"
#include "const.h"

extern struct minix_super_block sb;

/**
 * Clear all fields in an inode.
 */
static void wipe_inode(struct minix_inode *inode)
{
	int i;

	inode->i_mode = 0;
	inode->i_uid = 0;
	inode->i_size = 0;
	inode->i_time = 0;
	inode->i_gid = 0;
	inode->i_nlinks = 0;

	inode->i_num = 0;
	inode->i_dirty = FALSE;

	for(i = 0; i < NR_ZONE_NUMS; i++)
		inode->i_zone[i] = NO_ZONE;		
}

/**
 * Create empty minix inode.
 */
static struct minix_inode *mk_inode(inode_nr i_num)
{
	struct minix_inode *inode;

	inode = (struct minix_inode *) calloc(1, sizeof(struct minix_inode));
	inode->i_num = i_num;
	inode->i_dirty = FALSE;

	return inode;
}

/**
 * Read or write the given inode to it's disk block. 
 *
 * rw_flag set to READ to read
 * rw_flag set to WRITE to write
 */
static void rw_inode(struct minix_inode *i, int rw_flag)
{
	int i_block;		/* block inode is in */
	int i_block_offset;	/* byte offset of inode in block */
	struct minix_block *blk;/* blk containing inode */

	i_block = 2 + sb.s_imap_blocks + sb.s_zmap_blocks;
	i_block += ((i->i_num - 1) * INODE_SIZE) / BLOCK_SIZE;
	i_block_offset = ((i->i_num - 1) * INODE_SIZE) % BLOCK_SIZE;

	debug("read_inode(%d): reading inode from block %d offset %d...", 
		i->i_num, i_block, i_block_offset);

	blk = get_block(i_block, TRUE);

	if(rw_flag == READ) {
		memcpy((void *)i, (void *) (blk->blk_data + i_block_offset), 
			INODE_SIZE);
	}
	else {
		memcpy((void *) (blk->blk_data + i_block_offset), (void *) i,
			INODE_SIZE);
		blk->blk_dirty = TRUE;
	}

	put_block(blk, INODE_BLOCK);
	i->i_dirty = FALSE;
}


struct minix_inode *get_inode(inode_nr i_num)
{
	struct minix_inode *i, *free_slot;
	free_slot = NO_INODE;

	for(i = &inode_table[0]; i < &inode_table[NR_INODES]; i++) {
		if(i->i_count > 0) {
			if(i->i_num == i_num) {
				debug("get_inode(%d): found in inode table",
					i_num);
				/* found our inode */
				i->i_count++;
				return i;
			}
		}
		else {
			/* inode slot is free, remember for later */
			free_slot = i;
		}
	}

	if(free_slot == NO_INODE)
		panic("get_inode(%d): no free slots in inode table to read in "
			"read in inode", i_num);

	debug("get_inode(%d): reading inode from disk. storing in table entry "
		"at %p", i_num, free_slot);

	free_slot->i_num = i_num;
	free_slot->i_count = 1;
	rw_inode(free_slot, READ);
	return free_slot;
}
	
void put_inode(struct minix_inode *inode)
{
	inode->i_count--;
	if(inode->i_count < 0) panic("put_inode(%d): i_count decremeted to "
		"%d!", inode->i_num, inode->i_count);
	if(inode->i_count == 0) {
		/* no one is using inode, we can free it now */
		if(inode->i_dirty == TRUE) {
			debug("put_inode(%d): inode is dirty. writing...",
				inode->i_num);
			 rw_inode(inode, WRITE);
		}
		else {
			debug("put_inode(%d): inode isn't dirty. no need to "
				"write", inode->i_num);
		}
	}
	else {
		debug("put_inode(%d): inode still in use. i_count=%d",
			inode->i_num, inode->i_count);
	}
	
	// else inode is still in use
}


struct minix_inode *alloc_inode(void)
{
	struct minix_inode *inode;	
	inode_nr i_num;			
 
	debug("alloc_inode(): attempting to allocate free inode...");

	i_num = (inode_nr) alloc_bit(sb.imap, 0);
	if(i_num < 0) 
		panic("alloc_inode(): no free inodes available");

	inode = get_inode(i_num);
	wipe_inode(inode);
	inode->i_num = i_num;
	inode->i_dirty = FALSE;
	
	return inode;
}

void inode_print(struct minix_inode *inode)
{
	int i; 
	printf("========================= INODE =========================\n");
	printf("#               = %u\n", inode->i_num);
	printf("dirty?		= %s\n", 
		inode->i_dirty == TRUE ? "yes" : "no");
	printf("i_mode (u16)    = %u\n", inode->i_mode);
	printf("i_nlinks (u8)   = %u\n", inode->i_nlinks);
 	printf("i_uid (u16)     = %u\n", inode->i_uid);
 	printf("i_gid (u8)      = %u\n", inode->i_gid);
	printf("i_size (u32)    = %ub\n", inode->i_size);
 	printf("i_time (u32)    = %u\n", inode->i_time);
 	
	for(i=0; i < 9; i++) {
 		printf("i_zone[%d]      = %u\n", i, inode->i_zone[i]);
	}
	printf("=========================================================\n");
}