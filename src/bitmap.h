#ifndef _MINIX_BITMAP
#define _MINIX_BITMAP

/**
 * A generic bitmap.
 */
struct generic_bitmap {
	int num_blocks;			/* # of blocks to hold the bitmap */
	struct minix_block **blocks;	/* the bitmap blocks */
	int num_bits;			/* # of bits in bitmap */
};

void bitmap_print(struct generic_bitmap *bmap);
int alloc_bit(struct generic_bitmap *bitmap, int origin);
void free_bit(struct generic_bitmap *bitmap, int bit);

#endif
