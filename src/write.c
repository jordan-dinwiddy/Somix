#include <errno.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include "types.h"
#include "superblock.h"
#include "comms.h"
#include "const.h"
#include "cache.h"
#include "inode.h"
#include "write.h"

extern struct minix_super_block sb;

/**
 * Inserts a directory entry with inode_number and filename in the given
 * directory 'p_dir'
 */
static int dir_add(struct minix_inode *p_dir, const char *filename, 
	inode_nr i_num)
{
	int b;
	int c_pos = 0;
	struct minix_block *block;
	int i;
	int found_slot = 0;
	char *dentry_name;
	inode_nr *dentry_inode_nr;

	int existing_slots = p_dir->i_size / DENTRY_SIZE;
	int new_slots = 1;

	while((b = read_map(p_dir, c_pos)) != NO_ZONE) {
		block = get_block(b, TRUE);

		for(i = 0; i < BLOCK_SIZE; i += DENTRY_SIZE) {
			dentry_inode_nr =  (inode_nr *) (block->blk_data + i);
			if(*dentry_inode_nr == NO_INODE) {
				dentry_name = block->blk_data + i + 2;
				found_slot = 1;
				break;
			}
			new_slots++;
		}
		if(found_slot) break;
		c_pos += BLOCK_SIZE;
	}

	if(!found_slot) {
		/* we're going to have to add a new block to the directory to
  		 * hold our extra data. */
		debug("dir_add(%d, \"%s\", %d): no free slots in directory. "
			"extending...", p_dir->i_num, filename, i_num);
		
		if((block = new_block(dir, dir->i_size)) == NULL)
			panic("dir_add(...): unable to extend directory");
		
		dentry_inode_nr = (inode_nr *) (block->blk_data);
		dentry_name = block->blk_data + 2;
	}

	*dentry_inode_nr = inode_num;
	memset(dentry_name, 0x00, 30);
	strncpy(dentry_name, filename, 30);
	printf("minix_dentry_add(): Successfully inserted directory entry\n");

	if(new_slots > existing_slots) {
		printf("minix_dentry_add(): increasing directory size from %d "
			"to %d\n", dir->i_size, new_slots * DENTRY_SIZE);
		dir->i_size = DENTRY_SIZE * new_slots;
	}
	else {
		printf("* new_slots=%d existing_slots=%d\n", new_slots, existing_slots);
	}
	/* need to flush the block that was modified and the inode */
	put_inode(dir);
	put_block(block);
	return 1;
}



/**
 * Allocates a new zone/block for use.
 */
zone_nr alloc_zone(zone_nr near_zone)
{
	int bit = near_zone - (sb.s_firstdatazone - 1);
	int b;			/* bit we are allocated... hopefully */
	zone_nr z;		

	debug("alloc_zone(%d): looking for free bit near bit %d...", 
		near_zone, bit);
	
	if((b = alloc_bit(sb.zmap, bit)) < 0) 
		panic("alloc_zone(%d): no free zones available", near_zone);

	z = b + (sb.s_firstdatazone - 1);

	return z;
}

/**
 * Write a new zone into an inode.
 *
 * The function works out the relative zone for the files byte position given by
 * 'pos' and adjusts the mappings for the inode 'inode' so that this zone points
 * to the zone specified by 'new_zone'.
 *
 */
int write_map(struct minix_inode *inode, int pos, zone_nr new_zone)
{
	/* the byte position given indicates the new_zone to add */
	zone_nr zone = pos / BLOCK_SIZE;
	int excess, index;
	zone_nr *zp;		/* zone containing indirect mappings */
	zone_nr *dbl_zp;	/* zone containting double indirect mappings */

	char new_ind = FALSE;	/* if new indirect mapping block created */
	char new_dbl = FALSE;	/* if new dbl indirect mapping block created */
	struct minix_block *blk = NULL;

	/* is the zone we're adding a direct zone? */
	if(zone < NR_DZONE_NUM) {
		inode->i_zone[zone] = new_zone;
		inode->i_dirty = TRUE;	
		return 1;
	}

	/* otherwise the zone we're adding is indirect or double indirect */
	excess = zone - NR_DZONE_NUM;
	if(excess < NR_INDIRECTS) {
		/* the zone we're adding is indirect */
		zp = &inode->i_zone[NR_DZONE_NUM];
	}
	else {
		/* the zone we're adding is double indirect */
		dbl_zp = &inode->i_zone[NR_DZONE_NUM+1];
		if(*dbl_zp == NO_ZONE) {
			/* we dont have a double indirect mapping zone */
			*dbl_zp = alloc_zone(inode->i_zone[0]);
			if(*dbl_zp == NO_ZONE)
				return -ENOSPC;		/* out of space */

			new_dbl = TRUE;
		}

		excess -= NR_INDIRECTS;
		index = excess / NR_INDIRECTS;	/* ix into dbl indirect map */
		excess = excess % NR_INDIRECTS;	/* ix into indirect map */
	
		/* this should be ensured by minix_write_buf anyway */
		if(index >= NR_INDIRECTS) return -EFBIG;	
		
		/* if we're creating a new double indirect mapping zone then 
 		 * we don't have to read in block *dbl_zp from disk. */
		blk = get_block(*dbl_zp, new_dbl == TRUE ? FALSE : TRUE);
		if(new_dbl == TRUE) zero_block(blk);
	
		zp = &((zone_nr *)blk->blk_data)[index];
	}
	
	if(*zp == NO_ZONE) {
		/* we need to allocate a block to store all our indirect zone
		 * numbers. */
		*zp = alloc_zone(inode->i_zone[0]);

		if(blk != NULL) blk->blk_dirty = TRUE;
		if(*zp == NO_ZONE) {
			if(blk != NULL) put_block(blk, INDIRECT_BLOCK);
			return -ENOSPC;		/* out of space */
		}
		new_ind = TRUE;		/* we're creating a new indirect block */
		
	}
	put_block(blk, INDIRECT_BLOCK);		/* release dbl ind mappings,
						 * block may be dirty. */

	/* zp points to the indirect block.
 	 * TODO: can we be sure zp is still there? we've already called 
 	 * put_block(). */
	blk = get_block(*zp, new_ind == TRUE ? FALSE : TRUE);
	if(new_ind == TRUE) zero_block(blk);

	debug("write_map(...): setting index %d in indirect map to point to "
		"new zone %d", excess, new_zone);

	((zone_nr *)blk->blk_data)[excess] = new_zone;
	blk->blk_dirty = TRUE;
	put_block(blk, INDIRECT_BLOCK);
	
	/* its up to the calling routing to put the inode */
	return 1;
}



/**
 * Allocates a new inode, makes a directory entry for it in the directory
 * pointed to by the last but one component of the given path and initialises
 * the inode.
 */
struct minix_inode *new_node(struct minix_inode *parent, const char *filename, 
	mode_t mode)
{
	struct minix_inode *new_inode;

	new_inode = alloc_inode();

	new_inode->i_mode = (u16) mode;
	new_inode->i_uid =  500;
	new_inode->i_gid = 0;
	new_inode->i_time = time(NULL);
	new_inode->i_nlinks++;
	new_inode->i_dirty = TRUE;

	debug("new_node(\"%s\"): inserting entry \"%d-%s\" into directory "
		"inode %d...", filename, new_inode->i_num, filename, 
		parent->i_num);

	dir_add(parent, filename, new_inode->i_num);

	if(S_ISDIR(mode)) {
		debug("new_node(\"%s\"): new node is a directory, "
			"initing...", filename);
		
		/* this will allocate a first block to the directory too */
		init_dir(new_inode, parent);
	}
	else {
		debug("new_node(\"%s\"): new node is not a directory, "
			"not pre-allocating blocks.", filename);
	}

	/* TODO: when do we put inode? */
	return new_inode;
}

/*
 * Returns the zone corresponding to the given byte offset 'pos' within the 
 * file.
 *
 * if 'pos' is greater than the file size then a new zone will be allocated to
 * the file to hold bytes at position 'pos'.
 */
struct minix_block *new_block(struct minix_inode *inode, int pos)
{
	zone_nr z, near_z;
	struct minix_block *retval;
	char new_zone = FALSE;

	if((z = read_map(inode, pos)) == 0) {
		/* no block currently allocated for this byte offset */
		if(inode->i_size == 0) 
			near_z = sb.s_firstdatazone;
		else
			near_z = inode->i_zone[0];

		z = alloc_zone(z);
		new_zone = TRUE;

		/* attempt to write the new zone number to the inode. This
		 * function will deal with placing the new zone number in the
		 * direct, indirect or double indirect slots. */
		if(write_map(inode, pos, z) < 0) {
			debug("new_block(inode=%d, %d): unable to write map",
			inode->i_num, pos);
			return NULL;
		}

		
	}

	/* if we allocated a new block then we don't need to read from disk, we
 	 * we just need to zero it. */
	retval = get_block(z, new_zone == TRUE ? FALSE : TRUE);
	if(new_zone == TRUE) zero_block(retval); 
	return retval;
}

/**
 * Writes 'size' bytes from 'buf' to data contents of inode 'inode' starting at
 * 'offset'.
 */
int write_buf(struct minix_inode *inode, const char *buf, size_t size, off_t offset)
{
	int nbytes = size;	/* number of bytes left to write */
	int off;		/* offset within a particular zone */
	int chunk;		/* the number of bytes we are writing to a 
				 * particular zone. */
	int pos = offset;	/* where we are writing current chunk to
				 * relative to a files data. */
	int so_far = 0;		/* bytes written so far */
	int ret;		

	debug("minix_write_buf(inode=%d, %p, %d, %d):\n", inode->i_num, buf, 
		(int)size, (int)offset);

	/* operation not permitted - writing 0 bytes */
	if(size == 0) return -EPERM;

	if(offset > sb.s_max_size - size)
		return -EFBIG;	/* cannot grow file over max file size */

	while(nbytes > 0) {
		off = pos % BLOCK_SIZE;
		
		/* the number of bytes we're gonna write to the zone */
		chunk = MIN(nbytes, (BLOCK_SIZE - off));

		/* write 'chunk' bytes to the inode. starting at file position=
		 * 'pos' from the buffer starting at 'buf+so_far' */
		ret = write_chunk(inode, pos, chunk, buf+so_far);
		
		if(ret < 0) {
			debug("minix_write_buf(...): something when wrong while"
				" writing chunk %d\n", chunk);
			return ret;
		}

		nbytes -= chunk;	/* we have chunk bytes less to write */
		pos += chunk;		/* we've advanced by chunk bytes */
		so_far += chunk;	/* keep count of how much written */
	}

	/* if we've increased the file size then update inode */
	if(pos > inode->i_size)
		inode->i_size = pos;

	/* update mod time */
	inode->i_time = time(NULL);
	inode->i_dirty = TRUE;	
	
	return so_far;
}

/**
 * Write the first 'chunk' bytes from 'buf' to the block at position 'pos'.
 *
 * Writing a chunk will never span more than a single block.
 * There 'chunk' is always <= BLOCK_SIZE.
 *
 */
int write_chunk(struct minix_inode *inode, int pos, int chunk, 
	const char *buf)
{
	int zone = pos / BLOCK_SIZE;
	int off = pos % BLOCK_SIZE;
	int b_num = 0;
	struct minix_block *blk;

	debug("minix_write_chunk(inode=%d,%d,%d,%p): writing %d bytes from "
		"buffer starting at %p to file offset %d (zone=%d, offset=%d)\n",
		inode->i_num, pos, chunk, buf, chunk, buf, pos, zone, off);

	/* lookup the block corresponding to file position 'pos' */
	b_num = read_map(inode, pos);	

	if(!b_num) {
		/* the file does not have a block allocated for this position
		 * yet. We should allocate one now. */
		debug("minix_write_chunk(...): allocating new block for file "
			"position=%d...\n", pos);
		blk = new_block(inode, pos);	/* TODO: this should mark inode
						 * as dirty since it modifies */
		if(blk == NULL) {
			/* no more space */
			return -ENOSPC;
		}
	}
	else {
		/* if we are writing a full blocks worth of data we do not
		 * need to bother reading in block b_num before re-writing. */
		blk = get_block(b_num, chunk == BLOCK_SIZE ? FALSE : TRUE);
	}

	/* cpy 'chunk' bytes to blk->data+off from 'buf' */
	memcpy(blk->blk_data+off, buf, chunk);	

	put_block(blk, DATA_BLOCK);
	return 1;
}

