#include <stdlib.h>
#include "types.h"
#include "inode.h"

inode_nr dir_search(struct minix_inode *inode, const char *file);
int minix_read(struct minix_inode *inode, char *buf, size_t size, off_t offset);
zone_nr read_map(struct minix_inode *inode, int byte_offset);
