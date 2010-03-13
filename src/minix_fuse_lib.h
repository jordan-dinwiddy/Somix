#define MINIX_SUPER_MAGIC    0x137F          /* original minix fs */
#define MINIX_SUPER_MAGIC2   0x138F          /* minix fs, 30 char names */
#define MINIX2_SUPER_MAGIC   0x2468	     /* minix V2 fs */
#define MINIX2_SUPER_MAGIC2  0x2478	     /* minix V2 fs, 30 char names */

#define BLOCK_SIZE 1024

#define NR_ZONE_NUMS 9			/* zone numbers in an inode */
#define NO_ZONE 0			/* indicates an empty inode zone entry */
#define NR_DZONE_NUM 7 			/* Number of direct blocks */
#define NR_INDIRECTS (BLOCK_SIZE/2)	/* Number of indirect blocks */
#define DENTRY_SIZE 32			/* The size of a directory entry */
#define INTS_PER_BLOCK (BLOCK_SIZE/sizeof(int)) /* number of ints that can be 
					* stored in a block. */
#define INT_BITS (sizeof(int) << 3)	/* number of bits in an int */
#define BITS_PER_BLOCK (INTS_PER_BLOCK * INT_BITS)

/* test not sure if i need these?? */
#define HAVE_SETXATTR 1
#define HAVE_GETXATTR 1

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))

#define TRUE 1
#define FALSE 0


typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;


struct minix_block {
	char data[BLOCK_SIZE];
	int block_num;		/* block number */
};

struct minix_super_block {
        u16 s_ninodes;
        u16 s_nzones;
        u16 s_imap_blocks;
        u16 s_zmap_blocks;
        u16 s_firstdatazone;
        u16 s_log_zone_size;
        u32 s_max_size;
        u16 s_magic;
        u16 s_state;
        u32 s_zones;
};


struct minix_disk_inode {
        u16 i_mode;
        u16 i_uid;
        u32 i_size;
        u32 i_time;
        u8  i_gid;
        u8  i_nlinks;
        u16 i_zone[9];
};


struct minix_inode {
        u16 i_mode;
        u16 i_uid;
        u32 i_size;
        u32 i_time;
        u8  i_gid;
        u8  i_nlinks;
        u16 i_zone[9];

	/* fields not present on disk */
	u16 i_num;	/* inode number */
};

/**
 * A generic bitmap.
 */
struct generic_bitmap {
	int num_blocks;			/* the number of blocks that make up this bitmap */
	struct minix_block **blocks;	/* the blocks that make up the bitmap */
	int num_bits;			/* number of bits in bitmap */
};

/**
 * To store filesystem information in memory.
 */
struct minix_fuse_fs {
	char *device_name;			/* device name */
	int fd;					/* file descriptor for device */
	struct minix_super_block *sb;		/* in-memory cpy of 
						 * superblock */
	struct generic_bitmap *inode_bmap;	/* ptr to in-memory inode bitmap */
	struct generic_bitmap *zone_bmap;	/* ptr to in-memory zone bitmap */
	struct minix_inode *root_inode;		/* ptr to root inode */
};


struct dentry {
	u16 inode_num;
	char name[30];
	struct dentry *next;
};

int bit(char * addr,unsigned int nr);
int setbit(char * addr,unsigned int nr);
int clrbit(char * addr,unsigned int nr);

int minix_openfs(char *device_name, struct minix_fuse_fs *fs);
void print_buf(void *buf, int size);
void minix_print_sb(void);
void minix_print_inode(struct minix_inode *inode);

struct minix_inode *get_inode(int inode);
int put_inode(struct minix_inode *inode);
//int get_block(int block_num, void *buf);
struct minix_block *mk_block(int block_num);
void zero_block(struct minix_block *blk);
struct minix_block *get_block(int block_num);
int put_block(struct minix_block *block);

int read_map(struct minix_inode *inode, int zone_num);
int read_map_bytes(struct minix_inode *inode, int byte);

void print_block_dentries(struct minix_block *block);

void print_dentries(struct minix_inode *inode);

u16 dir_block_lookup(struct minix_block *block, char *name);
u16 lookup(struct minix_inode *inode, char *name);

int get_path_comp(const char *path, char *comp, int n);

struct minix_inode *minix_open_file(struct minix_inode *inode, const char *path);
int path_count_comp(const char *path);

int minix_load_bitmaps(void);
struct generic_bitmap *load_bitmap(int block_start, int num_blocks);

void print_bitmap(struct generic_bitmap *bitmap);

int alloc_bit(struct generic_bitmap *bitmap, int origin);

void free_bit(struct generic_bitmap *bitmap, int bit);

struct minix_inode *alloc_inode(void);

struct minix_inode *minix_last_dir(const char *path);

struct minix_inode *new_node(const char *path, mode_t mode);

int alloc_bit_2(struct generic_bitmap *bit, int origin);

int get_last_path_comp(const char *path, char *comp);



int alloc_zone(int near_zone);
int minix_dentry_add(struct minix_inode *dir, const char *filename, u16 inode_num);

int minix_flush_bitmaps(void);
int minix_flush_bitmap(struct generic_bitmap *bm);
struct minix_block *new_block(struct minix_inode *inode, int pos);

int init_dir(struct minix_inode *i, struct minix_inode *p_i);

int minix_write_buf(struct minix_inode *inode, const char *buf, size_t size, off_t offset);
int minix_write_chunk(struct minix_inode *inode, int pos, int chunk, const char *buf);

