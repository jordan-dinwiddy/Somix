/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall `pkg-config fuse --cflags --libs` hello.c -o hello
*/

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse_opt.h>
#include <fuse_lowlevel.h>
#include <stddef.h>
#include "minix_fuse_lib.h"


struct minix_fuse_fs fs;		/* representation of minix fs */

/* for command line options */
static struct options {
	char *device_name;
} options;

static struct fuse_opt options_desc[] =
{
	{"-dev=%s", offsetof(struct options, device_name), 0}
};

static int hello_getattr(const char *path, struct stat *stbuf)
{
	int res = 0;
	struct minix_inode *inode;

	printf("getattr(\"%s\", ...)\n", path);
	memset(stbuf, 0, sizeof(struct stat));
	
	inode = minix_open_file(fs.root_inode, path);
	if(inode == NULL) {
		printf("getattr(\"%s\", ...): cannot find item\n", path);
		return -ENOENT;
	}

	stbuf->st_mode = inode->i_mode;
	stbuf->st_nlink = inode->i_nlinks;
	stbuf->st_size = inode->i_size;
	stbuf->st_uid = inode->i_uid;
	stbuf->st_gid = inode->i_gid;
	stbuf->st_atime = inode->i_time;
	stbuf->st_mtime = inode->i_time;
	stbuf->st_ctime = inode->i_time;
	
return res;
}

static int hello_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{

	struct minix_block *block;
	struct minix_inode *inode = (struct minix_inode *) fi->fh;
	printf("readdir(\"%s\", , , %d, ): inode=%d\n", path, inode->i_num, 
		(int)offset);
	
	int z = 0;
	int blk = 0;
	int i;
	u16 *inode_num;
	while((blk = read_map(inode, z++)) != 0) {
		printf("reading block %d from inode %d...\n", blk, inode->i_num);
		block = get_block(blk);
		for(i = 0; i < BLOCK_SIZE; i += DENTRY_SIZE) {
			inode_num = (u16 *) (block->data + i);
			if(*inode_num == 0) continue;	// if inode is zero then no entry
			printf("filling in \"%s\"...\n", block->data+i+2);
			filler(buf, block->data+i+2, NULL, 0);
			/* ignoring inode for the moment */
		}
	}

	printf("end of readdir\n");

	return 0;
}

static int minix_open(const char *path, struct fuse_file_info *fi)
{
	struct minix_inode *inode;
	
	printf("open(%s,...):\n ", path);
	if((inode = minix_open_file(fs.root_inode, path)) == NULL) {
		printf("FAILED\n");
		return -ENOENT;
	}

	printf("open(%s,...): open successful. Inode %d is open\n", path, 
		inode->i_num);

	/* set file handle to point to inode */
	fi->fh = (unsigned long) inode;

	printf("Open success. Inode:\n");
	minix_print_inode(inode);
	return 0;
}

//
//static int hello_read(const char *path, char *buf, size_t size, off_t offset,
//		      struct fuse_file_info *fi)
//{
//	size_t len;
//	(void) fi;
//	printf("Reading...\n");
//
//	len = strlen(hello_str);
//	if (offset < len) {
//		if (offset + size > len)
//			size = len - offset;
//		memcpy(buf, hello_str + offset, size);
//	} else
//		size = 0;
//
//	return size;
//}
//
//

int minix_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	printf("create(\"%s\", %d, fi):\n", path, (int) mode );

	printf("create(\"%s\",...): file type = %s\n", path,
		S_ISREG(mode) ? "regular" : "other");
	struct minix_inode *inode = new_node(path, mode);
	if(inode == NULL) {
		/* could not create */
		printf("create(\"%s\", ...): could not create new file\n", path);
		return -ENOSPC;
	}

	printf("create(\"%s\",...): created new file. Inode=%d\n", path,
		 inode->i_num);

	fi->fh = (unsigned long) inode;
	return 0;
}

int minix_write(const char *path, const char *buf, size_t size, off_t offset, 
	struct fuse_file_info *fi)
{
	printf("minix_write(%s, %p, %d, %d, fi):\n", path, buf, (int) size, 
		(int) offset);
	struct minix_inode *inode = (struct minix_inode *) fi->fh;
	
	if(inode == NULL) {
		printf("minix_write(..): error, no file handle available\n");
		return 0;
	}

	printf("minix_write(..): inode=%d\n", inode->i_num);
	return minix_write_buf(inode, buf, size, offset);
	
	return (int) size;
}

static int hello_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	u16 zone;
	int blk = 0;
	struct minix_inode *inode = (struct minix_inode *) fi->fh;
	struct minix_block *block;
	int nbytes = size;	/* bytes remaining to be read */
	int current_position = offset;
	int zone_offset;
	int chunk;
	int bytes_so_far = 0;	/* bytes read so far */

	if(nbytes > inode->i_size) 
		nbytes = inode->i_size;

	printf("read: %s. %d bytes starting at %d\n", path, (int) size, (int) offset);
	while(nbytes > 0) {
		zone_offset = current_position % BLOCK_SIZE;
		if(nbytes < (BLOCK_SIZE - zone_offset) )
			chunk = nbytes;
		else
			chunk = (BLOCK_SIZE - zone_offset);
	
		zone = current_position / BLOCK_SIZE;

		printf("Copying bytes %d -> %d from zone %d... \n", zone_offset, chunk, zone);
		if(!(blk = read_map(inode, zone))) {
			printf("read(..): failed to read zone %d\n", zone);
			return bytes_so_far;
		}

		if((block = get_block(blk)) == NULL) {
			printf("read(..): failed to get block %d\n", blk);
			return bytes_so_far;
		}

		printf("memcpy\n");
		memcpy(buf+bytes_so_far, block->data+zone_offset, chunk);

		bytes_so_far += chunk;
		current_position += chunk;
		nbytes -= chunk;
		printf("read(...): bytes so far=%d\n", bytes_so_far);
	}


		
	return bytes_so_far;
}

int minix_opendir(const char *path, struct fuse_file_info *file_info)
{
	printf("opendir \"%s\"...\n", path);
	return 0;
}

int minix_mkdir(const char *path, mode_t mode)
{
	mode += S_IFDIR;
	printf("mkdir(\"%s\",%d)\n", path, (int) mode);

	new_node(path, mode);
	return 0;
} 

int minix_setxattr(const char *path, const char *name, const char *value, size_t size, 
		int flags)
{
	printf("setxattr(\"%s\", \"%s\", \"%s\", %d, %d):\n",
		path, name, value, (int) size, flags);
	return 0;
}

int minix_getxattr(const char *path, const char *name, char *value, size_t size)
{
	printf("getxattr(\"%s\", \"%s\", \"%s\", %d):\n",
		path, name, value, (int) size);

	return 0;
}

int minix_listxattr(const char *path, char *list, size_t size)
{
	printf("listxattr(\"%s\", \"%s\", %d):\n",
		path, list, (int) size);

	return 0;
}

/**
 * Update the last access time of the given object from tv[0] and the last
 * modification time from tv[1]. Times are in nanosecond resolution.
 *
 * The touch command requires this functionality.
 */
int minix_utimens(const char *path, const struct timespec tv[2])
{
	printf("utimens(\"%s\", ...):\n", path);

	return 0;
}

/*
 * Change the size of a file.
 *
 * Needed b y read/write file systems because recreating a file will first
 * truncate it.
 */
int minix_truncate(const char *path, off_t size)
{
	printf("truncate(\"%s\", %d):\n", path, (int) size);

	return 0;
}

int minix_release(const char *path, struct fuse_file_info *fi)
{
	struct minix_inode *inode = (struct minix_inode *) fi->fh;

	if(inode == NULL) {
		printf("release(\"%s\",...): cannot get hold of file handle\n",
			 path);
		return -1;
	}

	printf("release(\"%s\",...): releasing inode %d\n", path, inode->i_num);

	return 0;
}

static struct fuse_operations hello_oper = {
//	.init		= minix_init,  // WE do this work in main since we want
//	to exit if we cannot open the device and superblock.
	.getattr	= hello_getattr,
	.readdir	= hello_readdir,
	.open		= minix_open,
	.read		= hello_read,
	.opendir	= minix_open,
	.mkdir		= minix_mkdir,
	.write		= minix_write,
	.create		= minix_create,
	.setxattr	= minix_setxattr,
	.getxattr	= minix_getxattr,
	.listxattr	= minix_listxattr,
	.utimens	= minix_utimens,
	.truncate	= minix_truncate,
	.release	= minix_release,
};

int main(int argc, char *argv[])
{
	int ret;

	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	memset(&options, 0, sizeof(struct options));

	/* parse the command line options */
	if(fuse_opt_parse(&args, &options, options_desc, NULL) == -1)
		return -1;
	
	if(!minix_openfs(options.device_name, &fs))
		/* failed to open the device file and read superblock */
		return -1;

	ret = fuse_main(args.argc, args.argv, &hello_oper, NULL);

	fuse_opt_free_args(&args);
	return ret;
}
