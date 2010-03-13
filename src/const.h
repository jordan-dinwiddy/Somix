/* minix magic numbers */
#define MINIX_SUPER_MAGIC    0x137F	/* original minix fs */
#define MINIX_SUPER_MAGIC2   0x138F	/* minix fs, 30 char names */
#define MINIX2_SUPER_MAGIC   0x2468	/* minix V2 fs */
#define MINIX2_SUPER_MAGIC2  0x2478	/* minix V2 fs, 30 char names */

#define BLOCK_SIZE 1024			/* bytes per block */
#define NO_ZONE 0                       /* an empty inode zone entry */
#define NO_BLOCK 0			/* indicates an empty block */
#define DENTRY_SIZE 32                  /* bytes per directory entry */

#define NR_ZONE_NUMS 9                  /* zone entries per inode */
#define NR_DZONE_NUM 7                  /* direct zones per inode */
#define NR_INDIRECTS (BLOCK_SIZE/2)     /* zones per indirect zone */




#define INTS_PER_BLOCK \
	(BLOCK_SIZE/sizeof(int)) 	/* number of ints per block */
#define INT_BITS (sizeof(int) << 3)     /* number of bits in an int */
#define BITS_PER_BLOCK \
	(INTS_PER_BLOCK * INT_BITS)

#define TRUE 1
#define FALSE 0

/* handy macros */
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))


/* test not sure if i need these?? */
#define HAVE_SETXATTR 1
#define HAVE_GETXATTR 1

/* buffer cache */
#define NR_BUFS	1024		/* blocks in buffer cache */
#define NR_BUF_HASH 128		/* size of buffer hash table. power of 2 */
#define BLOCK_ALIGN 1024	/* the alignment of the address for the data 
				 * portion of a minix_block */
