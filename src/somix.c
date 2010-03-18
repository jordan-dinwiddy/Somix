#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <fuse_opt.h>
#include <fuse_lowlevel.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include "const.h"
#include "types.h"
#include "superblock.h"
#include "inode.h"
#include "comms.h"
#include "path.h"
#include "read.h"
#include "write.h"
#include "cache.h"
#include "mount.h"

extern struct minix_super_block sb;

/* for command line options */
static struct options {
	char *device_name;
} options;

static struct fuse_opt options_desc[] =
{
	{"-dev=%s", offsetof(struct options, device_name), 0}
};

static int somix_getattr(const char *path, struct stat *stbuf)
{
	int res = 0;
	struct minix_inode *inode;

	debug("getattr(\"%s\", ...)", path);
	memset(stbuf, 0, sizeof(struct stat));


	
	inode = resolve_path(sb.root_inode, path, PATH_RESOLVE_ALL);
	if(inode == NULL) {
		debug("getattr(\"%s\", ...): cannot find item", path);
		return -ENOENT;
	}
	debug("filling in and returning...");
	debug("mode=%d nlink=%d size=%d...", inode->i_mode, inode->i_nlinks, inode->i_size);
	stbuf->st_mode = inode->i_mode;
	stbuf->st_nlink = inode->i_nlinks;
	stbuf->st_size = inode->i_size;
	stbuf->st_uid = inode->i_uid;
	stbuf->st_gid = inode->i_gid;
	stbuf->st_atime = inode->i_time;
	stbuf->st_mtime = inode->i_time;
	stbuf->st_ctime = inode->i_time;
	
	if(inode != sb.root_inode)
		put_inode(inode);
	return res;
}

static int somix_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
	struct minix_inode *d_inode;	/* the directory we want to read */
	struct minix_block *blk;	/* to hold directory contents */
	zone_nr	z;			/* current zone of directory we're on */
	int c_pos = 0;			/* current position in directory */
	int i;				/* current position in directory block */

	d_inode = (struct minix_inode *) fi->fh;

	inode_nr *i_num;
	debug("readdir(\"%s\", ...):", path);
	while((z = read_map(d_inode, c_pos)) != 0) {
		blk = get_block(z, TRUE);
		for(i = 0; i < BLOCK_SIZE; i += DENTRY_SIZE) {
			i_num = (inode_nr *) (blk->blk_data + i);
			if(*i_num == NO_INODE) 
				continue;	/* ignore these entries */

			filler(buf, blk->blk_data + i + 2, NULL, 0);
			/* ignoring inode for the moment */
		}
		put_block(blk, DIR_BLOCK);
		c_pos += BLOCK_SIZE;
	}


	return 0;
}

static int somix_open(const char *path, struct fuse_file_info *fi)
{
	struct minix_inode *inode;
	debug("open...");

	if((inode = resolve_path(sb.root_inode, path, 
		PATH_RESOLVE_ALL)) == NULL) {
		return -ENOENT;
	}

	/* set file handle to point to inode */
	fi->fh = (unsigned long) inode;

	return 0;
}

int somix_release(const char *path, struct fuse_file_info *fi)
{
	struct minix_inode *inode = (struct minix_inode *) fi->fh;

	if(inode == NULL) {
		debug("release(\"%s\", ...): cannot release. "
			"file does not appear to be open", path);
		return -1;
	}

	put_inode(inode);
	debug("release(\"%s\", ...): inode released", path);

	return 0;
}

static int somix_read(const char *path, char *buf, size_t size, off_t offset,
	struct fuse_file_info *fi)
{
	struct minix_inode *inode = (struct minix_inode *) fi->fh;

	if(inode == NULL) {
		debug("read(\"%s\", ...): appear the specified file is not open",
			path);
		return -EIO;	/* I/O error */
	}

		
	debug("read(\"%s\", ...): inode(%d) open", path, inode->i_num);

	return minix_read(inode, buf, size, offset);
}

int somix_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	debug("create(\"%s\", ...):", path);

	int n = path_cnt_cmpos(path);
	struct minix_inode *p_dir;
	struct minix_inode *new_i;
	char filename[FILENAME_SIZE];

	if(path_get_last_cmpo(path, filename) == 0) 
		return -1;	/* 0 components in path = no file specified */

	p_dir = resolve_path(sb.root_inode, path, n - 1);

	debug("create(\"%s\", ...): attempting to insert \"%s\" into "
		"directory %d...", path, filename, p_dir->i_num); 
	
	new_i = new_node(p_dir, filename, mode);

	/* nasty hack until we implement inode cache.
 	 * it's possible that resolve_path will return the same
 	 * inode we give it (sb.root_inode) if we resolve the path /
 	 */
	if(p_dir != sb.root_inode)
		put_inode(p_dir);

	fi->fh = (unsigned long) new_i;
	debug("create(...): complete");
	return 0;
}

void somix_destroy(void * v)
{
	/* flush everything */
	debug("somix_destroy(): unmounting...");
	minix_unmount();
	debug("somix_destory(): finished");
}

static struct fuse_operations somix_oper = {
/* we do the job of .init in main since we want to exit gracefully if anything
 * goes wrong during init. */
	.getattr	= somix_getattr,
	.readdir	= somix_readdir,
	.open		= somix_open,
	.opendir	= somix_open,
	.release	= somix_release,
	.read		= somix_read,
	.create		= somix_create,
	.destroy	= somix_destroy,
/*
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
*/
};

int main(int argc, char *argv[])
{
	int ret;

	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	memset(&options, 0, sizeof(struct options));

	/* parse the command line options */
	if(fuse_opt_parse(&args, &options, options_desc, NULL) == -1)
		return -1;
	
	minix_mount(options.device_name);

	ret = fuse_main(args.argc, args.argv, &somix_oper, NULL);

	fuse_opt_free_args(&args);
	return ret;
}