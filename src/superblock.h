#ifndef _MINIX_SUPERBLOCK
#define _MINIX_SUPERBLOCK

#include "bitmap.h"
#include "inode.h"

struct minix_super_block_disk {
	/* on disk fields */
        u16 s_ninodes;
        u16 s_nzones;
        u16 s_imap_blocks;
        u16 s_zmap_blocks;
        u16 s_firstdatazone;
        u16 s_log_zone_size;
        u32 s_max_size;
        u16 s_magic;
        u16 s_state;
        u32 s_zones;
};

struct minix_super_block {
	/* on disk fields */
        u16 s_ninodes;
        u16 s_nzones;
        u16 s_imap_blocks;
        u16 s_zmap_blocks;
        u16 s_firstdatazone;
        u16 s_log_zone_size;
        u32 s_max_size;
        u16 s_magic;
        u16 s_state;
        u32 s_zones;

	/* in memory only fields */
	char *device_name;			/* device name */
	struct generic_bitmap *imap;		/* pointer to inode bitmap */
	struct generic_bitmap *zmap;		/* pointer to zone bitmap */
	struct minix_inode *root_inode;		/* pointer to root inode */

};

#endif
