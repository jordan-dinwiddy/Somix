#include <stdio.h>
#include "const.h"
#include "mount.h"
#include "inode.h"

#define DEVICE "TEST1.IMG"

int main(void)
{
	minix_mount(DEVICE);

	struct minix_inode *i1 = get_inode(1);
	inode_print(i1);
	i1->i_size++;
	i1->i_dirty = TRUE;
	put_inode(i1);
	minix_unmount();
	printf("finished\n");
	
	return 0;
}
