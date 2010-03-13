#include <stdio.h>
#include <stdlib.h>
#include "minix_fuse_lib.h"

struct minix_fuse_fs fs;

int main(int argc, char **argv)
{
	char *device = argv[1];
	int inode = atoi(argv[2]);

	printf("printing inode %d on device=%s...\n", inode, device);

	if(!minix_openfs(device, &fs)) {
		printf("failed to open device\n");
		return -1;
	}

	struct minix_inode *i = get_inode(inode);
	minix_print_inode(i);
	return 1;
}
