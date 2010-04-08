#include <stdio.h>
#include "const.h"
#include "cache.h"
#include "comms.h"
#include "bitmap.h"

static inline int bit(char * addr,unsigned int nr) 
{
	return (addr[nr >> 3] & (1<<(nr & 7))) != 0;
}

static inline int setbit(char * addr,unsigned int nr)
{
	int __res = bit(addr, nr);
	addr[nr >> 3] |= (1<<(nr & 7));
	return __res != 0;
}

static inline int clrbit(char * addr,unsigned int nr)
{
	int __res = bit(addr, nr);
	addr[nr >> 3] &= ~(1<<(nr & 7));
	return __res != 0;
}

/**
 * Print the contents of a given bitmap to stdout.
 */
void bitmap_print(struct generic_bitmap *bmap)
{
	int i;
	int block_nr, block_offset; 

	for(i = 0; i < bmap->num_bits; i++) {
		block_nr = i / BITS_PER_BLOCK;
		block_offset = i % BITS_PER_BLOCK;
		
		printf("Bit %d): ", i);
		if(bit(bmap->blocks[block_nr]->blk_data, block_offset))
			printf("SET\n");
		else
			printf("NOT SET\n");
	}

}

/**
 * Attempts to allocate and return a bit in the given bitmap that is as close 
 * to the specified 'origin' bit as possible.
 */
int alloc_bit(struct generic_bitmap *bitmap, int origin)
{
	register unsigned k;		/* current word being scanned */
	register int a;			/* the return bit */
	register int *wptr, *wlim;	/* word pointers */
	int i, b, w, o, block_count;
	struct minix_block *bp;

	if(origin >= bitmap->num_bits) origin = 0;

	b = origin >> 13;	/* the block where searching starts */
	o = origin - (b << 13); /* the bit offset into the block */
	w = o/INT_BITS;		/* the word offset into the block */
	
	block_count = bitmap->num_blocks;
	while(block_count--) {
		bp = bitmap->blocks[b];			/* block to search */
		wptr = ((int *) bp->blk_data) + w;	/* addr of start word */
		wlim = ((int *) bp->blk_data) + 
				INTS_PER_BLOCK;	/* the final address of block */
		a = w * INT_BITS;
		while(wptr != wlim) {
			/* loop through all words in block */
			if((k = (unsigned) *wptr) != (unsigned) ~0) {
				/* word at wptr has a free bit */
				for(i = 0; i < INT_BITS; i++) {
					if(!bit((char *)wptr, i)) {
						/* bit i @ word wptr is free */
						a += (b * INTS_PER_BLOCK * 
							INT_BITS) + i;

						if(a >= bitmap->num_bits) {
							wptr = wlim - 1;
							break;
						}
						setbit((char *)wptr, i);
						bp->blk_dirty = TRUE;
						return a;
					}
				}
			}

			wptr++;			/* move on to next word */
			a += INT_BITS;
		}
		/* wrap if necessary */
		if(++b == bitmap->num_blocks) b = 0;
		w = 0;
	}

	/* no free bits found */
	return -1;
}



/*
 * Free's a bit in the given gitmap.
 */
void free_bit(struct generic_bitmap *bitmap, int bit_num)
{
	int bits_per_block = BLOCK_SIZE * 8;
	int block = bit_num / bits_per_block;

	int block_bit = bit_num - (block * bits_per_block);

	if(!bit(bitmap->blocks[block]->blk_data, block_bit)) {
		/* bit is already free, something strange is going on */
		panic("free_bit(%d): bit is already free", bit_num);
	}

	clrbit(bitmap->blocks[block]->blk_data, block_bit);
	bitmap->blocks[block]->blk_dirty = TRUE;
}

