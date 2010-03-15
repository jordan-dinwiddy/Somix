#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "const.h"
#include "types.h"
#include "comms.h"
#include "cache.h"
#include "superblock.h"
#include "inode.h"

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

struct minix_inode *get_inode(inode_nr i_num)
{
	int i_block;		/* block inode is located in */
	int i_block_offset;	/* byte offset of inode in this block */
	struct minix_block *blk;
	struct minix_inode *inode;

	/* calculate disk block and offset inode is located at */
	i_block = 2 + sb.s_imap_blocks + sb.s_zmap_blocks;
	i_block += ((i_num - 1) * INODE_SIZE) / BLOCK_SIZE;
	i_block_offset = ((i_num - 1) * INODE_SIZE) % BLOCK_SIZE;

	debug("get_inode(%d): retrieving inode from block %d at offset %d...", 
		i_num, i_block, i_block_offset);
	blk = get_block(i_block, TRUE);		/* retrieve block from disk */

	/* copy inode from the cache. TODO: this is not ideal. maybe implement
	 * seperate inode cache to remove need for memcpy. */
	inode = mk_inode(i_num);
	memcpy(inode, blk->blk_data + i_block_offset, INODE_SIZE);
	put_block(blk, INODE_BLOCK); 
	
	return inode;
}

void put_inode(struct minix_inode *inode)
{
	int i_block;	
	int i_block_offset;
	struct minix_block *blk;

	/* if inode has been modified we'll have to write to disk */
	if(inode->i_dirty == TRUE) {
		i_block = 2 + sb.s_imap_blocks + sb.s_zmap_blocks;
		i_block += ((inode->i_num - 1) * INODE_SIZE) / BLOCK_SIZE;
		i_block_offset = ((inode->i_num - 1) * INODE_SIZE) % 
			BLOCK_SIZE;

		debug("put_inode(%d): putting inode to block %d at offset "
			"%d...", inode->i_num, i_block, i_block_offset);
		blk = get_block(i_block, TRUE);		
		memcpy(blk->blk_data + i_block_offset, inode, INODE_SIZE);
		blk->blk_dirty = TRUE;		/* mark inode block as dirty*/
		put_block(blk, INODE_BLOCK);
	}
	
	debug("put_inode(%d): freeing inode...", inode->i_num);
	free(inode);
}

struct minix_inode *alloc_inode(void)
{
	struct minix_inode *inode;	
	inode_nr i_num;			
 
	debug("alloc_inode(): attempting to allocate free inode...");

	i_num = (inode_nr) alloc_bit(sb.imap, 0);
	if(i_num < 0) 
		panic("alloc_inode(): no free inodes available");

	inode = mk_inode(i_num);

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
