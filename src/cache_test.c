#include <stdio.h>
#include "const.h"
#include "cache.h"

#include <unistd.h>
extern struct minix_block *front;

int main(void)
{
	open_blk_device("TEST3.IMG");	
	init_cache();

	struct minix_block *b1, *b2, *b3;

	b1 = get_block(1, TRUE);
	b2 = get_block(1002, TRUE);
	b3 = get_block(6091, TRUE);

	b2 = get_block(1002, TRUE);

	b1->blk_dirty = TRUE;
	b2->blk_dirty = TRUE;
	b3->blk_dirty = TRUE;
	printf("\n");
	put_block(b1, SUPER_BLOCK);
	put_block(b2, DATA_BLOCK);
	put_block(b3, DATA_BLOCK);
	put_block(b2, DATA_BLOCK);
	printf("\n");
	printf("syncing...\n");
	int k = sync_cache();
	printf("%d blocks written to disk\n", k);
	return 0;
}
