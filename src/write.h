#include <stdlib.h>
#include "types.h"
#include "inode.h"

zone_nr alloc_zone(zone_nr near_zone);
void free_zone(zone_nr z);
void truncate(struct minix_inode *inode);
int write_map(struct minix_inode *inode, int pos, zone_nr new_zone);
struct minix_inode *new_node(struct minix_inode *parent, const char *filename, 
	mode_t mode);
struct minix_block *new_block(struct minix_inode *inode, int pos);
int write_buf(struct minix_inode *inode, const char *buf, size_t size, 
	off_t offset);
int dir_delete(struct minix_inode *p_dir, const char *filename);
int unlink(const char *path);
