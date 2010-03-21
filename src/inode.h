#ifndef _MINIX_INODE
#define _MINIX_INODE

#include "types.h"
#include "const.h"

/**
 * Minix inode as it appears on disk.
 */
struct minix_inode_disk {
        u16 i_mode;
        u16 i_uid;
        u32 i_size;
        u32 i_time;
        u8  i_gid;
        u8  i_nlinks;
        u16 i_zone[9];
};

/**
 * Minix inode as it appear in memory
 */
struct minix_inode {
        u16 i_mode;
        u16 i_uid;
        u32 i_size;
        u32 i_time;
        u8  i_gid;
        u8  i_nlinks;			/* # of things pointing to this file */
        u16 i_zone[9];

	/* in memory only fields */
	inode_nr i_num;			/* inode number */
	char i_dirty;			/* whether inode has been modified */
	int i_count;			/* # of users of inode */
};

struct minix_inode inode_table[NR_INODES];

struct minix_inode *get_inode(inode_nr i_num);
void put_inode(struct minix_inode *inode);
struct minix_inode *alloc_inode(void);
void free_inode(inode_nr i_num);
void inode_print(struct minix_inode *inode);
void print_inode_table(void);

#endif
