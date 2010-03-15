#define _GNU_SOURCE 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "minix_fuse_lib.h"
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <errno.h> /* ENOSPC etc */
//
char sb_buffer[BLOCK_SIZE];		// Super block buffer
char *sb_buffer_2;

extern struct minix_fuse_fs fs;

int minix_openfs(char *device_name, struct minix_fuse_fs *fs)
{
	fs->device_name = device_name;

	if((fs->fd = open(device_name, (O_RDWR | O_DIRECT))) < 0) {
		printf("Unable to open device %s\n", device_name);
		return 0;
	}
	else {
		printf("Opened device for direct i/o\n");
		printf("fd=%d\n", fs->fd);
		printf("errno=%d\n", errno);
		printf("strerror=%s\n", strerror(errno));
	}
	if(lseek(fs->fd, 1024, SEEK_SET) != 1024) {
		printf("Unable to seek to superblock on device %s.\n", 
			device_name);
		return 0;
	}

/*	fs->sb = (struct minix_super_block *) 
			malloc(sizeof(struct minix_super_block));
	if(read(fs->fd, fs->sb, sizeof(struct minix_super_block)) !=
		sizeof(struct minix_super_block)) {
		printf("Unable to read in superblock on device %s.\n",
			device_name);
		return 0;
	}
*/
	int ret = 0;
	if((ret=posix_memalign((void **) &sb_buffer_2, BLOCK_SIZE, BLOCK_SIZE)) != 0) {
		printf("minix_openfs(\"%s\"),...): could not align memory "
			"for superblock buffer.\n", device_name);	
		printf("return=%d\n", ret);
		return 0;
	}
	else {
		printf("successfully got mem aligned address %p\n", sb_buffer_2);
	}

	if((ret=read(fs->fd, sb_buffer_2, BLOCK_SIZE)) != BLOCK_SIZE) {
		printf("minix_openfs(\"%s\",...): unable to read in "
			"superblock.\n", device_name);
		printf("return=%d\n", ret);
		printf("errno=%d\n", errno);
		printf("strerror=%s\n", strerror(errno));
		return 0;
	}

	fs->sb = (struct minix_super_block *) sb_buffer_2;

	switch(fs->sb->s_magic) {
		case MINIX_SUPER_MAGIC:
			printf("Identified Minix V1 File System\n");
			break;
		case MINIX_SUPER_MAGIC2:
			printf("Identified Minix V1 File System (30 char names)\n");
			break;
		case MINIX2_SUPER_MAGIC:
			printf("Identified Minix V2 File System\n");
			break;
		case MINIX2_SUPER_MAGIC2:
			printf("Identified Minix V2 File System (30 char names)\n");
			break;
		default:
			printf("Unable to identify Minix file system\n");
			return 0;
	}

	
	if(!(fs->root_inode = get_inode(1)))
		return 0;	/* failed to load root inode */

	minix_print_inode(fs->root_inode);

	if(!minix_load_bitmaps())
		return 0;	/* failed to load bitmaps */
	
	return 1;
}	

int minix_load_bitmaps(void)
{
	int zone_bmap_start = 2 + (fs.sb->s_imap_blocks);

	if(!(fs.inode_bmap = load_bitmap(2, fs.sb->s_imap_blocks)))
		return 0;	/* failed to load inode bitmap block(s) */
	fs.inode_bmap->num_bits = fs.sb->s_ninodes;

	if(!(fs.zone_bmap = load_bitmap(zone_bmap_start, fs.sb->s_zmap_blocks)))
		return 0;	/* failed to load zone bitmap block(s) */
	fs.zone_bmap->num_bits = fs.sb->s_nzones;

	printf("minix_load_bitmaps(): Inode and Zone bitmaps successfully "
		"loaded\n");
	return 1;
}

int minix_flush_bitmaps(void)
{
	minix_flush_bitmap(fs.inode_bmap);
	minix_flush_bitmap(fs.zone_bmap);

	return 1;
}

int minix_flush_bitmap(struct generic_bitmap *bm) {
	int i;
	printf("flush_bitmap(...): bitmap holds %d bits\n", bm->num_bits);
	for(i = 0; i < bm->num_blocks; i++) {
		printf("flush_bitmap(...): flushing block %d of bitmap...\n",
			i);
		put_block(bm->blocks[i]);
	}

	return 1;
}
		
struct generic_bitmap *load_bitmap(int block_start, int num_blocks)
{
	int i;
	struct generic_bitmap *retval = calloc(1, sizeof(struct generic_bitmap));

	retval->num_blocks = num_blocks;
	retval->blocks = calloc(num_blocks, sizeof(struct minix_block *));
	
	
	for(i = 0; i < num_blocks; i++) {
		/* load block i of the bitmap and store in the correct offset 
 		 * in the buffer. */
		printf("load_bitmap(%d, %d): loading block %d of bitmap...\n", 
			block_start, num_blocks, i);
		retval->blocks[i] = get_block(block_start+i);	
	}

	return retval;
}

int bit(char * addr,unsigned int nr) 
{
	return (addr[nr >> 3] & (1<<(nr & 7))) != 0;
}

int setbit(char * addr,unsigned int nr)
{
	int __res = bit(addr, nr);
	addr[nr >> 3] |= (1<<(nr & 7));
	return __res != 0;
}

int clrbit(char * addr,unsigned int nr)
{
	int __res = bit(addr, nr);
	addr[nr >> 3] &= ~(1<<(nr & 7));
	return __res != 0;
}

void print_bitmap(struct generic_bitmap *bitmap)
{
	int i;
	int block_nr, block_offset; 

	for(i = 0; i < bitmap->num_bits; i++) {
		block_nr = i / BITS_PER_BLOCK;
		block_offset = i % BITS_PER_BLOCK;
		
		printf("Bit %d): ", i);
		if(bit(bitmap->blocks[block_nr]->data, block_offset))
			printf("SET\n");
		else
			printf("NOT SET\n");
	}

}

void print_buf(void *buf, int size)
{
	printf("Printing first %d bytes of buffer at %p...\n", size, buf);

	int i = 0;
	u8 my_byte;
	while(i < size) {
		if(i%4==0 && i!= 0) printf("\n");
		my_byte = ((u8 *)buf)[i];
		if(isgraph(my_byte) || my_byte == 0x20) 
			printf("%2x [%-3u](%6c)    ",my_byte, my_byte, my_byte);
		else 
			printf("%2x [%-3u]( ?)    ", my_byte, my_byte);
		i++;
	}
}

void minix_print_sb(void)
{
	printf("======================= SUPERBLOCK =========================\n");
	printf("# inodes        = %u\n", fs.sb->s_ninodes);
	printf("# zones         = %u\n", fs.sb->s_nzones);
	printf("s_imap_blocks   = %u\n", fs.sb->s_imap_blocks);
	printf("s_zmap_blocks   = %u\n", fs.sb->s_zmap_blocks);
	printf("s_firstdatazone = %u\n", fs.sb->s_firstdatazone);
	printf("s_log_zone_size = %u\n", fs.sb->s_log_zone_size);
	printf("s_max_size	= %d\n", fs.sb->s_max_size);
	printf("s_state		= %u\n", fs.sb->s_state);
	printf("============================================================\n");
}


void minix_print_inode(struct minix_inode *inode)
{
	int i;

	printf("========================== INODE ===========================\n");
	printf("#		= %u\n", inode->i_num);
	printf("i_mode (u16)	= %u\n", inode->i_mode);
	printf("i_nlinks (u8)	= %u\n", inode->i_nlinks);
	printf("i_uid (u16)	= %u\n", inode->i_uid);
	printf("i_gid (u8)	= %u\n", inode->i_gid);
	printf("i_size (u32)	= %ub\n", inode->i_size);
	printf("i_time (u32)	= %u\n", inode->i_time);

	for(i=0; i < 9; i++) {
		printf("i_zone[%d]	= %u\n", i, inode->i_zone[i]);
	}
	printf("============================================================\n");

}


struct minix_inode *get_inode(int inode)
{
	int i_size = sizeof(struct minix_disk_inode);
	struct minix_inode *buf = (struct minix_inode *) calloc(1, i_size);
//struct minix_inode *buf;
//posix_memalign((void **) &buf, BLOCK_SIZE, BLOCK_SIZE);
	

	int inode_data_start = 2 * BLOCK_SIZE;		// skip boot block and sb
	inode_data_start += fs.sb->s_imap_blocks * BLOCK_SIZE; 	// skip inode bitmap
	inode_data_start += fs.sb->s_zmap_blocks * BLOCK_SIZE;	// skip zone bitmap

	int inode_block = 2 + fs.sb->s_imap_blocks + fs.sb->s_zmap_blocks;
	inode_block += ((inode-1) * i_size) / BLOCK_SIZE;
	int inode_block_offset = ((inode-1) * i_size) % BLOCK_SIZE;

	printf("get_inode(%d): inode is located in block %d offset %d\n", inode, 
		inode_block, inode_block_offset);

//inode--;					/* confusion over root inode */
	
	/**
 * 	inode_data_start += (inode-1) * i_size;		// Seek to correct inode offset

	printf("get_inode(%d): loading inode of length %db from offset %d...\n",
		  inode, i_size, inode_data_start);
	lseek(fs.fd, inode_data_start, SEEK_SET);
	if(read(fs.fd, buf, BLOCK_SIZE) != BLOCK_SIZE) {
		printf("get_inode(%d): failed to read all bytes for inode. "
			"exiting...\n", inode);
		close(fs.fd);
		exit(-1);
	}

	buf->i_num = inode;
	printf("get_inode(%d): successfully loaded inode.\n", inode);
	minix_print_inode(buf);
	printf("=================end of get_inode=====================\n");
	return buf;
*/

	struct minix_block *blk = get_block(inode_block);
	memcpy((void *) buf, (void *)blk->data+inode_block_offset, i_size);
	/* do something with this block! */
	buf->i_num = inode;
	return buf;
}

/*
 * Write inode to disk.
 */
int put_inode(struct minix_inode *inode)
{
	//int inode_disk_loc = 0;
	int i_size = sizeof(struct minix_disk_inode);

	printf("put_inode(inode=%d):\n", inode->i_num);

	int inode_block = 2 + fs.sb->s_imap_blocks + fs.sb->s_zmap_blocks;
	inode_block += ((inode->i_num-1) * i_size) / BLOCK_SIZE;
	int inode_block_offset = ((inode->i_num-1) * i_size) % BLOCK_SIZE;

	printf("put_inode(%d): inode located in block %d at offset %d\n", 
		inode->i_num, inode_block, inode_block_offset);

	struct minix_block *blk = get_block(inode_block);

	printf("put_inode(%d): copying inode to block...\n", inode->i_num);
	memcpy(blk->data+inode_block_offset, (void *) inode, i_size);

	/* save block with new inode in it */
	put_block(blk);


/*
	inode_disk_loc += 2 * BLOCK_SIZE;	
	inode_disk_loc += fs.sb->s_imap_blocks * BLOCK_SIZE;
	inode_disk_loc += fs.sb->s_zmap_blocks * BLOCK_SIZE;
	inode_disk_loc += (inode->i_num - 1) * i_size;

	printf("put_inode(inode=%d): writing inode back to disk location %d...\n",
		inode->i_num, inode_disk_loc);

	if(lseek(fs.fd, inode_disk_loc, SEEK_SET) != inode_disk_loc) {
		printf("put_inode(...): unable to seek to disk location.\n");
		return -1;
	}

	if(write(fs.fd, (void *) inode, i_size) != i_size) {
		printf("put_inode(...): unable to write inode data\n");
		return -1;
	}

	printf("put_inode(...): successfully wrote inode\n");
	printf("->\n");
	minix_print_inode(inode);	

*/

	return 1;
}

	

/**
 * Wipes all fields in the given inode.
 */
void wipe_inode(struct minix_inode *inode)
{
	int i;
	
	printf("wipe_inode(inode=%d): wiping inode...\n", inode->i_num);
	inode->i_mode = 0;
	inode->i_uid = 0;
	inode->i_gid = 0;
	inode->i_size = 0;
	inode->i_time = 0;
	inode->i_nlinks = 0;
	
	for(i = 0; i < NR_ZONE_NUMS; i++) 
		inode->i_zone[i] = NO_ZONE;
}

struct minix_inode *alloc_inode(void)
{
	struct minix_inode *retval;
	int inode_num = alloc_bit(fs.inode_bmap, 0);
//inode_num++;	/* TODO: fix this */
	if(inode_num < 0) {
		printf("alloc_inode(): no free inodes available.\n");
		return NULL;
	}

	printf("alloc_inode(): allocated inode=%d\n", inode_num);
	
	/* otherwise we have an inode */
	retval = get_inode(inode_num);

	/* wipe inode */
	/* TODO: could be more efficient here if only wiped fields no already
 	 * set. */
	wipe_inode(retval);

	return retval;
}


int alloc_zone(int near_zone)
{
	int bit = near_zone - (fs.sb->s_firstdatazone - 1);
	int b;
	int alloc_zone;

	printf("alloc_zone(%d): looking for free bit near bit %d...\n", 
		near_zone, bit);

	
	b = alloc_bit(fs.zone_bmap, bit);
	alloc_zone = b + (fs.sb->s_firstdatazone - 1);

	if(b < 0) {
		printf("alloc_zone(%d): no free zones available.\n", 
			near_zone);
		return NO_ZONE;
	}

	printf("alloc_zone(%d): allocated bit=%d (zone=%d)\n", near_zone, 
		b, alloc_zone);
	return alloc_zone;
}

/**
 * Create a new empty data block.
 */
struct minix_block *mk_block(int block_num)
{
	struct minix_block *blk;	
	blk = malloc(sizeof(struct minix_block));
	
	/* don't zero block here because most of the time it will be filled with
	 * a full block from disk (via get_block). For the occasions when we
	 * create a new block in memory and then write it, we should call
	 * zero_block(blk) */

	blk->block_num = block_num;

	return blk;
}

/*
 * Clear the data contents of a data block.
 */
void zero_block(struct minix_block *blk) 
{
	memset(blk->data, 0x00, BLOCK_SIZE);
}

struct minix_block *get_block(int block_num)
{
	int offset = block_num * BLOCK_SIZE;
	struct minix_block *block = mk_block(block_num);

	char * buf;
	posix_memalign((void **) &buf, BLOCK_SIZE, BLOCK_SIZE);

	printf("get_block(%d): attempting to retrieve block from disk "
		"location %d...\n", block_num, offset);

	if(lseek(fs.fd, offset, SEEK_SET) != offset) {
		/* seek failed */
		printf("get_block(%d): failed to seek to byte %d\n", 
			block_num, offset);
		exit(-1);
		return NULL;
	}

	if(read(fs.fd, buf, BLOCK_SIZE) != BLOCK_SIZE) {
		/* failed to read */
		printf("get_block(%d): failed to read from byte %d\n",
			block_num, offset);
		exit(-1);
	}

	memcpy(block->data, buf, BLOCK_SIZE);
	free(buf);
 
	return block;
}

int put_block(struct minix_block *block)
{
	int block_offset = block->block_num * BLOCK_SIZE;
	printf("put_block(%d): writing block to disk location %d...\n",
		 block->block_num, block_offset);

	char *buf;
	posix_memalign((void **) &buf, BLOCK_SIZE, BLOCK_SIZE);
	memcpy(buf, block->data, BLOCK_SIZE);
	
	if(lseek(fs.fd, block_offset, SEEK_SET) != block_offset) {
		printf("put_block(...): seek failed\n");
		return -1;
	}

	if(write(fs.fd, buf, BLOCK_SIZE) != BLOCK_SIZE) {
		printf("put_block(...): failed to write block\n");
		return -1;
	}

	printf("put_block(...): successfully wrote block to disk\n");
	free(buf);
	return 1;
}

	
/**
 * Loads the given block into the given buffer.
 * Returns 1 if success, 0 otherwise.
/
int get_block(int block_num, void *buf)
{
	int offset = block_num * BLOCK_SIZE;
	if(lseek(fs.fd, offset, SEEK_SET) != offset) {
		// failed to seek
		printf("get_block(%u, %p): FAILED TO SEEK\n", block_num, buf);
		return 0;
	}

	if(read(fs.fd, buf, BLOCK_SIZE) != BLOCK_SIZE) {
		//failed to read
		printf("get_block(%u, %p): FAILED TO READ\n", block_num, buf);
		return 0;
	}

	return 1;
}
*/

/*
 * Convert byte offset in file to the block containing that byte.
 */
int read_map_bytes(struct minix_inode *inode, int byte)
{
	int zone_num = byte / BLOCK_SIZE;
	return read_map(inode, zone_num);
}


/*
 * Given an inode and a zone number this function returns the device block
 * number holding that zone.
 */
int read_map(struct minix_inode *inode, int zone_num)
{
	struct minix_block *block;	
	int z;				// Zone number
	int excess;

	if(zone_num < NR_DZONE_NUM) {
		/* The zone exists in the inodes direct zones */
		if((z = inode->i_zone[zone_num]) == 0) 
			return 0;	// No direct block
		return z;
	}			
	excess = zone_num - NR_DZONE_NUM;
	if(excess < NR_INDIRECTS) {
		/* The zone exists in the inodes indirect zones. Set z to the
 		 * zone containing the indirect mappings. */
		z = inode->i_zone[NR_DZONE_NUM];	
	}
	else {
		/* The zone can be located via the double indirect block */
		if((z = inode->i_zone[NR_DZONE_NUM+1]) == 0)
			return 0;	// No Double indirect mapping
		excess -= NR_INDIRECTS;
		block = get_block(z);	// Load the double indirect mapping into z
		z = ((u16 *)block->data)[excess/NR_INDIRECTS];	// Set zone to zone containing single indirect mappings
		excess = excess % NR_INDIRECTS;
		//printf("offset in indirect = %d\n", excess);
	}

	/* z is zone number for single indirect block. excess is an index into
 	 * it. */
	if(z == 0) return 0;		// No indirect mapping
	block = get_block(z);		// Load the indirect mappings into z
	z = ((u16 *)block->data)[excess];	// set z to block containing data.
	return z;			// z could be zero
}

/**
 * Write a new zone into an inode.
 *
 *
 */
int write_map(struct minix_inode *inode, int pos, u16 new_zone)
{
	/* the byte position given indicates the new_zone to add */
	int zone = pos / BLOCK_SIZE;
	int excess, index;
	u16 *zp;	/* pointer to indirect zone number */
	u16 *dbl_zp;	/* the zone number of double indirect mapping zone */

	int new_ind = FALSE;	/* if new indirect mapping block created */
	int new_dbl = FALSE;	/* if new dbl indirect mapping block created */
	struct minix_block *blk = NULL;

	/* is the zone we're adding a direct zone? */
	if(zone < NR_DZONE_NUM) {
		inode->i_zone[zone] = new_zone;
		put_inode(inode);
		return 1;
	}

	/* otherwise the zone we're adding is indirect or double indirect */
	excess = zone - NR_DZONE_NUM;
	if(excess < NR_INDIRECTS) {
		/* the zone we're adding is indirect */
		zp = &inode->i_zone[NR_DZONE_NUM];
		printf("write_map(...): position=%d is stored in indirect zone."
			" indirect map located at zone=%d\n", pos, *zp);
	}
	else {
		/* the zone we're adding is double indirect */
		printf("write_map(...): position=%d is stored in double indirect "
			"zone\n", pos);
		
		dbl_zp = &inode->i_zone[NR_DZONE_NUM+1];
		if(*dbl_zp == NO_ZONE) {
			/* we dont have a double indirect mapping zone */
			*dbl_zp = alloc_zone(inode->i_zone[0]);
			if(*dbl_zp == NO_ZONE)
				return -ENOSPC;		/* out of space */

			new_dbl = TRUE;
		}

		excess -= NR_INDIRECTS;
		index = excess / NR_INDIRECTS;	/* index into dbl indirect map */
		excess = excess % NR_INDIRECTS;	/* index into indirect map */
	
		/* this should be ensured by minix_write_buf anyway */
		if(index >= NR_INDIRECTS) return -EFBIG;	
		
		/* whether to create a new block to store a double indirect
		 * mappings or load an existing one. */
		if(new_dbl) {
			blk = mk_block(*dbl_zp);
			zero_block(blk);
		}
		else {
			blk = get_block(*dbl_zp);
		}

		zp = &((u16 *)blk->data)[index];
	}
	
	if(*zp == NO_ZONE) {
		/* we need to allocate a block to store all our indirect zone
		 * numbers. */
		*zp = alloc_zone(inode->i_zone[0]);
		if(*zp == NO_ZONE) {
			return -ENOSPC;		/* out of space */
		}
		new_ind = TRUE;		/* we're creating a new indirect block */
		
		if(blk != NULL) {
			printf("write_map(...): attempting to save double "
				"indirect zone...\n");
			put_block(blk);		/* We've allocated a new indirect
						 * zone and stored in dbl map so
						 * save our dbl map */
		}
	}

	/* zp points to the indirect block */
	if(new_ind) {
		printf("write_map(...): creating new indirect map...\n");
		blk = mk_block(*zp);
		zero_block(blk);
	}
	else { 
		printf("write_map(...): loading existing indirect map...\n");
		/* indirect block already exists */
		blk = get_block(*zp);
	}

	printf("write_map(...): setting index %d in indirect map to point to "
		"new zone %d\n", excess, new_zone);

	((u16 *)blk->data)[excess] = new_zone;

	/* write changes to disk. */
	/* TODO: This is temporary. should really mark blocks as dirty and let
	 * cache flush them out later. */
	put_block(blk);		/* flush indirect block */
	put_inode(inode);	/* flush inode */

	return 1;
}



void print_block_dentries(struct minix_block *block)
{
	int i;
	u16 *inode_num;
	for(i=0; i<BLOCK_SIZE; i+=32) {
		inode_num = (u16 *) (block->data + i);
		if(*inode_num != 0) {
			printf("%-35s inode=%u\n", (char *)(block->data+i+2), 
				*inode_num);
		}
	}
}

void print_dentries(struct minix_inode *inode) {
	struct minix_block *block;

	int z = 0;
	int blk = 0;
	while((blk = read_map(inode, z++)) != 0) {
		block = get_block(blk);
		print_block_dentries(block);
	}
	
}

/**
 * Looks for the given file name in the data block specified by buf. This block
 * should contain a directory listing.
 * THINK MINIX BOOK CALLS THIS FUNCTION 'search_dir'
 */
u16 dir_block_lookup(struct minix_block *block, char *name) 
{
	int i;
	for(i = 0; i < BLOCK_SIZE; i += DENTRY_SIZE) {
		if(strcmp((char *)(block->data+2+i), name) == 0)
			return *((u16 *)(block->data + i));
	}
	return 0;
}

/**
 * Given a directory inode and a file name this function performs a linear
 * search through the directory contents in an attempt to find the given name.
 * Returns the inode corresponding to the file name or 0 if the file cannot be
 * found.
 * THINK MINIX BOOK CALLS THIS FUNCTION 'advance'
 */
u16 lookup(struct minix_inode *inode, char *name)
{

	struct minix_block *block;

	int z = 0;
	int blk = 0;
	u16 retval = 0;
	
	printf("lookup(inode=%d, \"%s\"):\n", inode->i_num, name);

	while((blk = read_map(inode, z++)) != 0) {
		block = get_block(blk);
		if((retval = dir_block_lookup(block, name)) != 0) {
			printf("lookup(inode=%d, \"%s\"): found item. inode=%d\n",
				inode->i_num, name, retval);
			return retval;	// We found the file
		}
	}

	// Exhausted all inode directory listing blocks and still could not find
	// file.
	printf("lookup(inode=%d, \"%s\": unable to find item\n", inode->i_num,
		name);
	return 0;
}

/**
 * Get the n'th path component and stores in comp
 * Return 0 indicates failure. Maybe the n'th component does not exist.
 */
int get_path_comp(const char *path, char *comp, int n)
{
	int offset = 0;				
	while(*(path + offset) == '/') offset++;	// Strip leading slashes

	/* skip past first n components in path. After skipping past n
	 * components path will be left pointing to the start of the
	 * n+1'th component exluding its leading slash. */
	for( ; *(path+offset) != '\0' && n != 0; ) {
		if(*(path+offset) == '/') {
			while(*(path+offset)=='/') offset++; // allows //foo///bar/
			n--;
			continue;
		}

		offset++;
	}

	if(n != 0) return 0;		/* path too short for n'th component */

	if(*(path+offset) == '\0') return 0;	/* We had a trailing slash */
	while(*(path+offset) != '/' && *(path+offset) != '\0') { 
		*comp = *(path+offset);
		offset++;
		comp++;
	}
	*comp = '\0';

	return 1;
}	

/**
 * Gets the final component of the given path and stores in comp.
 * If path_count_comp returns 0 for the given path then this function will
 * return 0 as an indication of an error. Otherwise it will return 1 to indicate
 * success.
 */
int get_last_path_comp(const char *path, char *comp) 
{
	int cnt = 0;
	if((cnt = path_count_comp(path)) > 0) {
		get_path_comp(path, comp, cnt-1);
		return 1;
	}
	return 0;
}



/**
 * Counts the number of path components in a given path. 
 * Examples:
 * 	- / has 0 components
 * 	- // has 0 components
 * 	- /home has 1 component
 * 	- //home has 1 component
 * 	- /home/foo/bar/foo has 4 components
 */
int path_count_comp(const char *path)
{
	int i = 0;
	while(*path != '\0') {
		while(*path == '/' && *path != '\0') path++;	/* eat slashes */
		if(*path != '\0') i++;
		while(*path != '/' && *path != '\0') path++;	/* eat component */
		
	}
	return i;
}

/**
 * Attempts to locate the inode for the file specified in path. The path is
 * relative to the directory given by inode.
 */
struct minix_inode *minix_open_file(struct minix_inode *inode, const char *path)
{	
	char string[32];
	int i = 0;
	u16 i_num;

	while(get_path_comp(path, string, i++)) {
		if((i_num = lookup(inode, string)) == 0) 
			return NULL;	/* failed to lookup the i'th component */

		
		/* TODO: Check we loaded inode okay */
		/* TODO: free inode first */
 		inode = get_inode(i_num);
	}

	return inode;
}

/**
 * Given an absolute path this function attempts to find the inode corresponding
 * to the last but one component of this path.
 * Examples:
 * 	path=/ returns the root inode
 * 	path=/foo returns the root inode
 * 	path=/foo/bar returns the inode for foo
 *
 */
struct minix_inode *minix_last_dir(const char *path)
{
	struct minix_inode *inode = fs.root_inode;

	int i,n;
	u16 i_num;
	char string[32];

	n = path_count_comp(path);

	if(n < 2) return inode;		/* return root inode */

	for(i = 0; i < (n-1); i++) {
		get_path_comp(path, string, i);
		if((i_num = lookup(inode, string)) == 0)
			return NULL;	/* failed to lookup i'th component */
		
		/* TODO: see minix_open_file */
		inode = get_inode(i_num);
	}

	return inode;
}

/**
 * Allocates a new inode, makes a directory entry for it in the directory
 * pointed to by the last but one component of the given path and initialises
 * the inode.
 */
struct minix_inode *new_node(const char *path, mode_t mode)
{
	struct minix_inode *p_dir = minix_last_dir(path);	/* parent dir */
	int n_components = path_count_comp(path);
	char filename[30];
	struct minix_inode *new_inode;
	
	if(n_components < 1) {
		printf("new_node(\"%s\",...): can't make node\n", path);
		exit(-1);
	}

	if(p_dir == NULL) {
		printf("new_node(%s, ...): cannot open parent directory\n", path);
		exit(-1);
	}


	get_path_comp(path, filename, n_components-1);	/* we'll insert entry for filename */

	new_inode = alloc_inode();
	if(new_inode == NULL) 
		return NULL;	/* no more inodes left */

	new_inode->i_mode = (u16) mode;
	new_inode->i_uid =  500;
	new_inode->i_gid = 0;
	new_inode->i_time = time(NULL);
	new_inode->i_nlinks++;

	printf("new_node(\"%s\"): inserting entry \"%d-%s\" into directory "
		"inode %d...\n", path, new_inode->i_num, filename, 
		p_dir->i_num);

	minix_dentry_add(p_dir, filename, new_inode->i_num);

	if(S_ISDIR(mode)) {
		printf("new_node(\"%s\", %d): new node is a directory, "
			"initing...\n", path, (int) mode);
		
		/* this will allocate a first block to the directory too */
		init_dir(new_inode, p_dir);
	}
	else {
		printf("new_node(\"%s\", %d): new node is not a directory, "
			"not pre-allocating blocks.\n", path, (int) mode);
	}

	/* flush new inode to disk */
	put_inode(new_inode);

	/* flush bitmaps */
	minix_flush_bitmaps();
	return new_inode;
}

/**
 * Init's a newly created directory by adding . and .. entries to it.
 */
int init_dir(struct minix_inode *i, struct minix_inode *p_i)
{
	printf("init_dir(%d, %d): initing directory...\n", i->i_num, 
		p_i->i_num);

	minix_dentry_add(i, ".", i->i_num);
	minix_dentry_add(i, "..", p_i->i_num);

	printf("init_dir(%d, %d): complete\n", i->i_num, p_i->i_num);

	return 1;
}

int minix_dentry_add(struct minix_inode *dir, const char *filename, u16 inode_num)
{

	int b;
	int z = 0;
	struct minix_block *block;
	int i;
	int found_slot = 0;
	char *dentry_name;
	u16 *dentry_inode_nr;
	printf("minix_dentry_add(%d, \"%s\", %d):\n", dir->i_num, filename, 
		inode_num);

	int existing_slots = dir->i_size / DENTRY_SIZE;
	int new_slots = 1;

	while((b = read_map(dir, z++)) != 0) {
		printf("minix_dentry_add(...): reading block %d from inode...\n", 
			b);
		block = get_block(b);

		for(i = 0; i < BLOCK_SIZE; i += DENTRY_SIZE) {
			printf("minix_dentry_add(...): looking at slot at %d\n", i);
			dentry_inode_nr =  (u16 *) (block->data + i);
			if(*dentry_inode_nr == 0) {
				printf("minix_dentry_add(...): slot is free\n");
				dentry_name = block->data + i + 2;
				found_slot = 1;
				break;
			}
			new_slots++;
		}
		if(found_slot) break;
	}

	if(!found_slot) {
		/* we're going to have to add a new block to the directory to
		 * hold our extra data. */
		printf("minix_dentry_add(...): no free slots available to add "
			"directory entry. \n");
		if((block = new_block(dir, dir->i_size)) == NULL) {
			printf("minix_dentry_add(...): unable to allocate new "
				"block\n");
			return -1;
		}
		dentry_inode_nr = (u16 *) (block->data);
		dentry_name = block->data + 2;
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
			
int alloc_bit(struct generic_bitmap *bitmap, int origin)
{
	register unsigned k;		/* current word being scanned */
	register int a;			/* the return bit */
	register int *wptr, *wlim;	/* word pointers */
	int i, b, w, o, block_count;
	struct minix_block *bp;

	if(origin >= bitmap->num_bits) origin = 0;

	b = origin >> 13;	/* the block where searching starts */
	o = origin - (b << 13); /* the bit offset into the block */
	w = o/INT_BITS;		/* the word offset into the block */
	
	block_count = bitmap->num_blocks;
	while(block_count--) {
		bp = bitmap->blocks[b];		/* the block to search */
		wptr = ((int *) &bp->data) + w;	/* the address of start word */
		wlim = ((int *) &bp->data) + 
				INTS_PER_BLOCK;	/* the final address of block */
		a = w * INT_BITS;
		while(wptr != wlim) {
			/* loop through all words in block */
			if((k = (unsigned) *wptr) != (unsigned) ~0) {
				/* word at wptr has a free bit */
				for(i = 0; i < INT_BITS; i++) {
					if(!bit((char *)wptr, i)) {
						/* bit i @ word wptr is free */
						a += (b * INTS_PER_BLOCK * 
							INT_BITS) + i;

						if(a >= bitmap->num_bits) {
							wptr = wlim - 1;
							break;
						}

						setbit((char *)wptr, i);
						return a;
					}
				}
			}

			wptr++;			/* move on to next word */
			a += INT_BITS;
		}
		/* wrap if necessary */
		if(++b == bitmap->num_blocks) b = 0;
		w = 0;
	}

	/* no free bits found */
	return -1;
}



/*
 * Free's a bit in the given gitmap.
 */
void free_bit(struct generic_bitmap *bitmap, int bit_num)
{
	int bits_per_block = BLOCK_SIZE * 8;
	int block = bit_num / bits_per_block;

	int block_bit = bit_num - (block * bits_per_block);

	if(!bit(bitmap->blocks[block]->data, block_bit)) {
		/* bit is already free, something strange is going on */
		printf("bit %d is already free.\n", bit_num);
		exit(-1);
	}

	clrbit(bitmap->blocks[block]->data, block_bit);
}

/*
 * Returns the block corresponding to the given byte position in the given
 * file/inode. If the byte position is too large for the number of blocks
 * allocated to the file then more blocks may be allocated.
 */
struct minix_block *new_block(struct minix_inode *inode, int pos)
{
	int b;
	int z;
	struct minix_block *retval;

	if((b = read_map_bytes(inode, pos)) == 0) {
		/* no block currently allocated for this byte offset */
		if(inode->i_size == 0) 
			z = fs.sb->s_firstdatazone;
		else
			z = inode->i_zone[0];

		if((b = alloc_zone(z)) == NO_ZONE) {
			printf("new_block(inode=%d, %d): unable to allocate "
				"new zone\n", inode->i_num, pos);
			return NULL;
		}

		/* attempt to write the new zone number to the inode. This
		 * function will deal with placing the new zone number in the
		 * direction, indirect or double indirect slots. */
		if(write_map(inode, pos, b) < 0) {
			printf("new_block(inode=%d, %d): unable to write map\n",
			inode->i_num, pos);
			return NULL;
		}

		
	}

	/* TODO: don't need to read from disk */
	retval = get_block(b);
	return retval;
}	



int minix_write_buf(struct minix_inode *inode, const char *buf, size_t size, off_t offset)
{
	int nbytes = size;	/* number of bytes left to write */
	int off;		/* offset within a particular zone */
	int chunk;		/* the number of bytes we are writing to a 
				 * particular zone. */
	int pos = offset;	/* where we are writing current chunk to
				 * relative to a files data. */
	int so_far = 0;		/* bytes written so far */
	int ret;		

	printf("minix_write_buf(inode=%d, %p, %d, %d):\n", inode->i_num, buf, 
		(int)size, (int)offset);

	/* operation not permitted - writing 0 bytes */
	if(size == 0) return -EPERM;

	if(offset > fs.sb->s_max_size - size)
		return -EFBIG;	/* cannot grow file over max file size */

	while(nbytes > 0) {
		off = pos % BLOCK_SIZE;
		
		/* the number of bytes we're gonna write to the zone */
		chunk = MIN(nbytes, (BLOCK_SIZE - off));

		/* write 'chunk' bytes to the inode. starting at file position=
		 * 'pos' from the buffer starting at 'buf+so_far' */
		ret = minix_write_chunk(inode, pos, chunk, buf+so_far);
		
		if(ret < 0) {
			printf("minix_write_buf(...): something when wrong while"
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
	put_inode(inode);

	/* TODO: fix this, it is just temporary at the mo*/
	minix_flush_bitmaps();
	return so_far;
}

/**
 * Write the first 'chunk' bytes from 'buf' to the block at position 'pos'.
 *
 * Writing a chunk will never span more than a single block.
 * There 'chunk' is always <= BLOCK_SIZE.
 *
 */
int minix_write_chunk(struct minix_inode *inode, int pos, int chunk, 
	const char *buf)
{
	int zone = pos / BLOCK_SIZE;
	int off = pos % BLOCK_SIZE;
	int b_num = 0;
	struct minix_block *blk;

	printf("minix_write_chunk(inode=%d,%d,%d,%p): writing %d bytes from "
		"buffer starting at %p to file offset %d (zone=%d, offset=%d)\n",
		inode->i_num, pos, chunk, buf, chunk, buf, pos, zone, off);

	/* lookup the block corresponding to file position 'pos' */
	b_num = read_map_bytes(inode, pos);	

	if(!b_num) {
		/* the file does not have a block allocated for this position
		 * yet. We should allocate one now. */
		printf("minix_write_chunk(...): allocating new block for file "
			"position=%d...\n", pos);
		blk = new_block(inode, pos);	/* TODO: this should mark inode
						 * as dirty since it modifies */
		if(blk == NULL) {
			/* no more space */
			return -ENOSPC;
		}
	}
	else {
		/* TODO: if we are writing a full blocks worth of data we do not
		 * need to bother reading in block b_num before re-writing. */
		blk = get_block(b_num);
	}

	/* cpy 'chunk' bytes to blk->data+off from 'buf' */
	memcpy(blk->data+off, buf, chunk);	

	put_block(blk);
	return 1;
}

 
		
