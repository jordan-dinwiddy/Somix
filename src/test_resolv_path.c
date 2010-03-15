#include <stdio.h>
#include "mount.h"
#include "inode.h"
#include "path.h"
#include "superblock.h"
#include "comms.h"

extern struct minix_super_block sb;

int main(int argc, char **argv)
{
	printf("attempting to mount TEST1.IMG...\n");

	minix_mount("TEST1.IMG");
	if(argc != 2)
		panic("usage: test_resolv_path [path]\n");
		
	printf("attempting to resolve path %s...\n", argv[1]);
	struct minix_inode *inode = resolve_path(sb.root_inode, argv[1], PATH_RESOLVE_ALL);
	
	if(inode == NULL)
		printf("failed\n");
	else
		printf("found! inode=%d\n", inode->i_num);

	minix_unmount();
	return 0;
}
