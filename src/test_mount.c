#include <stdio.h>
#include "cache.h"
#include "minix_fuse_lib.h"
#include "bitmap.h"
#include "mount.h"

extern struct minix_super_block sb;
int main(void)
{
	printf("attempting to mount TEST1.IMG...\n");

	minix_mount("TEST1.IMG");

	bitmap_print(sb.zmap);
	printf("allocating bit...\n");
	
	int b = alloc_bit(sb.zmap, 1);
	printf("got bit %d\n", b);

	bitmap_print(sb.zmap);
	minix_unmount();
	return 0;
}
