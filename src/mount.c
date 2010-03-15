#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "const.h"
#include "cache.h"
#include "comms.h"
#include "bitmap.h"
#include "superblock.h"
#include "inode.h"
#include "mount.h"

struct minix_super_block sb;

/**
 * Load and return a bitmap of size 'num_blocks' that starts at block offset
 * 'blk_offset'.
 */
static struct generic_bitmap *get_bitmap(int blk_offset, int num_blocks)
{
	int i;
	struct generic_bitmap *bmap = calloc(1, sizeof(struct generic_bitmap));
	bmap->num_blocks = num_blocks;
	bmap->blocks = (struct minix_block **) 
		malloc(num_blocks * sizeof(struct minix_block *));

	for(i = 0; i < num_blocks; i++) {
		debug("get_bitmap(%d, %d): loading bitmap block %d...", 
			blk_offset, num_blocks, i);
		bmap->blocks[i] = get_block(blk_offset + i, TRUE);
	}

	return bmap;
}

/**
 * Load and return the inode bitmap.
 */
static struct generic_bitmap *get_imap()
{
	int imap_offset = 2;		/* inode bmap comes after boot and 
					 * super blocks. */
	struct generic_bitmap *bmap;

	bmap = get_bitmap(imap_offset, sb.s_imap_blocks);
	bmap->num_bits = sb.s_ninodes;
	return bmap;
}

/**
 * Load and return the zone bitmap.
 */
static struct generic_bitmap *get_zmap()
{
	int zmap_offset = 2 + 
		sb.s_imap_blocks;	/* zone bmap comes after inode bmap */
	struct generic_bitmap *bmap;

	bmap = get_bitmap(zmap_offset, sb.s_zmap_blocks);
	bmap->num_bits = sb.s_nzones;
	return bmap;
}

/**
 * Reads the superblock from disk and copies to the global variable sb.
 */ 
static void read_super(void)
{
	struct minix_block *blk;

	debug("read_super(): attempting to read superblock...");
	blk  = get_block(SUPER_BLOCK_NR, TRUE);
	
	/* copy disk data to in memory super block */
	memcpy(&sb, blk->blk_data, sizeof(struct minix_super_block_disk));
	debug("read_super(): copying %d bytes to %p...", (int)sizeof(
		struct minix_super_block_disk), blk->blk_data);
	/* release the super block from the cache */
	put_block(blk, SUPER_BLOCK);
}

/**
 * Attempts to identify the type of minix file system currently mounted and
 * prints information to standard output.
 */
static void minix_print_version(void)
{
	switch(sb.s_magic) {
		case MINIX_SUPER_MAGIC:
			printf("identified minix v1 file system\n");
			break;
		case MINIX_SUPER_MAGIC2:
			printf("identified minix v1 file system (30 char "
				"names\n");
			break;
		case MINIX2_SUPER_MAGIC:
			printf("identified minix v2 file system\n");
			break;
		case MINIX2_SUPER_MAGIC2:
			printf("identified minix v2 file system (30 char "
				"names\n");
			break;
		default:
			printf("unable to identify minix file system\n");
	}
}


static void load_bitmaps(void)
{
	debug("load_bitmaps(): loading bitmaps...");
	sb.imap = get_imap();
	sb.zmap = get_zmap();
}

static void unload_bitmaps(void)
{
	int i;
	for(i = 0; i < sb.imap->num_blocks; i++) 
		put_block(sb.imap->blocks[i], IMAP_BLOCK);

	for(i = 0; i < sb.zmap->num_blocks; i++)
		put_block(sb.zmap->blocks[i], ZMAP_BLOCK);
}

/**
 * Attempts to mount a Minix file system located on the given device.
 */
void minix_mount(const char *device_name)
{
	open_blk_device(device_name);
	init_cache();
	read_super();

	minix_print_version();
	
	sb.device_name = malloc(strlen(device_name));
	strcpy(sb.device_name, device_name);
	load_bitmaps();
	sb.root_inode = get_inode(ROOT_INODE);

}

void minix_unmount(void)
{
	debug("minix_unmount(): unloading bitmaps...");
	unload_bitmaps();

	/* TODO: write superblock */
	debug("minix_unmount(): unloading superblock...");
	/* TODO: put_root inode */
	debug("minix_unmount(): syncing with disk...");
	sync_cache();
}
