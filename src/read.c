#include <stdlib.h>		/* size_t & off_t type defs */
#include <string.h>		/* memcpy def */
#include "types.h"
#include "const.h"
#include "cache.h"
#include "inode.h"
#include "comms.h"
#include "read.h"

/**
 * Looks for entry 'file' in the given directory contents. The inode is 
 * assumed to be of directory type.
 *
 * If the file is found it's inode number is returned, otherwise NO_INODE is
 * returned.
 */
inode_nr dir_search(struct minix_inode *inode, const char *file)
{
	struct minix_block *blk;	/* block belonging to inode */
	zone_nr z;			
	int c_pos = 0;		/* current position in scan of directory */
	int i;			/* current position in directory block */
	inode_nr retval;

	debug("dir_search(%d, \"%s\"): searching...", inode->i_num,
		file);
	while((z = read_map(inode, c_pos)) != NO_ZONE) {
		blk = get_block(z, TRUE);
		/* TODO: fix bug. shouldn't use BLOCK_SIZE */
		for(i = 0; i < BLOCK_SIZE; i+= DENTRY_SIZE) {
			if(strcmp((char *)(blk->blk_data + i + 2), file) == 0) {
				retval = *((inode_nr *)(blk->blk_data + i));
				put_block(blk, DIR_BLOCK);
				return retval;
			}
		}
		put_block(blk, DIR_BLOCK);
		c_pos += BLOCK_SIZE;
	}

	/* couldn't find directory entry */
	debug("dir_search(%d, \"%s\"): couldn't find entry", inode->i_num, 
		file);
	return NO_INODE;
}

/**
 * Reads 'size' bytes starting at 'offset' from the data of the given inode.
 * 
 * If successful buf[0] -> buf[size-1] will contain bytes read.
 *
 * Returns the number of bytes successfully read.
 */
int minix_read(struct minix_inode *inode, char *buf, size_t size, off_t offset)
{
	int sbytes = 0;		/* bytes read so far */
	int nbytes = size;	/* bytes remaining to be read */
	int c_pos = offset;	/* current position in file data */

	zone_nr z = 0;		/* the zone c_pos is located in */
	zone_nr z_data = 0;	/* the zone # on disk that holds c_pos */
	int z_offset = 0;	/* c_pos's offset in this zone */
	int chunk = 0;		/* # bytes we're reading from this zone */

	struct minix_block *blk;	/* current block we're reading */

	/* can't possibly read more than the file has to offer */
	if(nbytes > inode->i_size)
		nbytes = inode->i_size;

	while(nbytes > 0) {
		z = c_pos / BLOCK_SIZE;
		z_offset = c_pos % BLOCK_SIZE;
		chunk = MIN(nbytes, (BLOCK_SIZE - z_offset));

		/* if we try to read past file end then stop and just return
		 * bytes read so far. */
		if((z_data = read_map(inode, z * BLOCK_SIZE)) == NO_ZONE)
			return sbytes;

		blk = get_block(z_data, TRUE);

		/* copy the required contents (chunk bytes) of the block to 
 		 * correct position in user buffer. */
		memcpy(buf + sbytes, blk->blk_data + z_offset, chunk);

		put_block(blk, DATA_BLOCK);

		sbytes += chunk;	/* ++ bytes read so far */
		nbytes -= chunk;	/* -- bytes left to read */
		c_pos += chunk;		/* ++ current position in file data */
	}

	/* return the number of bytes we managed to read */
	return sbytes;
}

/*
 * Given an inode and a byte offset in a file this function returns the 
 * zone number on disk of the zone holding data at that offset.
 */
zone_nr read_map(struct minix_inode *inode, int byte_offset)
{
	int rel_z = byte_offset / BLOCK_SIZE;	/* file relative zone we want */
	zone_nr z;				/* zone number on disk */
	struct minix_block *blk;	/* may need to store zone mappings */
	int excess;

	if(rel_z < NR_DZONE_NUM) {
		/* The zone exists in the inodes direct zones */
		if((z = inode->i_zone[rel_z]) == NO_ZONE) 
			return NO_ZONE;	
		return z;
	}		
		
	excess = rel_z - NR_DZONE_NUM;
	if(excess < NR_INDIRECTS) {
		/* The zone exists in the inodes indirect zones. Set z to the
 		 * zone containing the indirect mappings. */
		z = inode->i_zone[NR_DZONE_NUM];	
	}
	else {
		/* The zone can be located via the double indirect block */
		if((z = inode->i_zone[NR_DZONE_NUM+1]) == NO_ZONE)
			return NO_ZONE;		/* no data at byte_offset */
		excess -= NR_INDIRECTS;
		
		/* get block containing dbl indirect mappings */
		blk = get_block(z, TRUE);

		/* set z to the zone containing the direct mappings */
		z = ((u16 *)blk->blk_data)[excess/NR_INDIRECTS];

		excess = excess % NR_INDIRECTS;
	
		put_block(blk, INDIRECT_BLOCK);	 
	}

	/* z is zone number for single indirect block. excess is an index into
 	 * it. */
	if(z == NO_ZONE) return NO_ZONE;	/* no data at byte_offset */
	blk = get_block(z, TRUE);		/* load indirect mappings */
	z = ((u16 *)blk->blk_data)[excess];	/* z now points to data zone */
	put_block(blk, INDIRECT_BLOCK);

	return z;				/* could be NO_ZONE */
}

