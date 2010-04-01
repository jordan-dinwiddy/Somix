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
			debug("somix_readdir(): adding \"%s\" to filler", 
				blk->blk_data+i+2);
			filler(buf, blk->blk_data + i + 2, NULL, 0);
			/* ignoring inode for the moment */
		}
		put_block(blk, DIR_BLOCK);
		c_pos += BLOCK_SIZE;
	}

	debug("readdir(): finished");
	return 0;
}

static int somix_open(const char *path, struct fuse_file_info *fi)
{
	struct minix_inode *inode;
	debug("open(\"%s\")", path);
	if((struct minix_inode *)fi->fh != NULL)
		panic("strange");
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

	debug("release(\"%s\")", path);

	if(inode == NULL) {
		debug("release(\"%s\", ...): cannot release. "
			"file does not appear to be open", path);
		return -1;
	}

	put_inode(inode);

	return 0;
}

static int somix_read(const char *path, char *buf, size_t size, off_t offset,
	struct fuse_file_info *fi)
{
	struct minix_inode *inode = (struct minix_inode *) fi->fh;
	
	debug("somix_read(): reading bytes %d -> %d from file \"%s\"...",
		(int) offset, (int) (offset + size), path);
	if(inode == NULL) {
		panic("read(\"%s\", ...): appear the specified file is not open",
			path);
		return -EIO;	/* I/O error */
	}

		
	debug("read(\"%s\", ...): inode(%d) open", path, inode->i_num);

	return minix_read(inode, buf, size, offset);
}

static int somix_write(const char *path, const char *buf, size_t size, 
	off_t offset, struct fuse_file_info *fi)
{
	struct minix_inode *inode = (struct minix_inode *) fi->fh;

	debug("somix_write(): writing bytes %d -> %d of file \"%s\"...",
		(int) offset, (int) (offset + size), path);
	if(inode == NULL) 
		panic("write(): called but no inode available");

	debug("write(\"%s\", %p, %d, %d, %d): writing...",
		path, buf, (int) size, (int) offset, inode->i_num);

	return write_buf(inode, buf, size, offset);
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

static int somix_truncate(const char *path, off_t offset)
{
	struct minix_inode *i;
	debug("somix_truncate(): truncating \"%s\" to %d bytes...", path, 
		(int) offset);

	i = resolve_path(sb.root_inode, path, PATH_RESOLVE_ALL);
	if(i == NULL)
		panic("truncate(\"%s\", %d): cannot resolve path",
			path, (int) offset);

	truncate(i);

	put_inode(i);

	return 0;
}

static int somix_unlink(const char *path)
{
	debug("somix_unlink(\"%s\")", path);

	if(!unlink(path))
		return -EIO;	/* TODO: return correct error */

	debug("somix_unlink(\"%s\"): complete", path);
	return 0;
}

static int somix_mkdir(const char *path, mode_t mode)
{
	struct minix_inode *i;
	struct minix_inode *new_i;

	char filename[FILENAME_SIZE];

	debug("somix_mkdir(\"%s\", %d): creating directory...", path, mode);
	mode += S_IFDIR;

	i = last_dir(path, filename);
	if(i == NULL) {
		debug("somix_mkdir(\"%s\", ...): connot find parent directory",
			path);
		return -ENOENT;
	}
	
	new_i = new_node(i, filename, mode);

	put_inode(i);
	put_inode(new_i);
	debug("somix_mkdir(): complete");
	return 0;
}

static int somix_rmdir(const char *path)
{
	debug("somix_rmdir(): removing directory \"%s\"...", path);

	if(!unlink(path))
		return -EIO;	/* TODO: return correct error */

	debug("somix_rmdir(\"%s\"): complete", path);
	return 0;
}

static int somix_rename(const char *old_path, const char *new_path)
{
	int ret;
	debug("somix_rename(): renaming \"%s\" to \"%s\"...", old_path, new_path);

	if((ret = rename(old_path, new_path)) != 1)
		return ret;

	return 0;
}

static int somix_releasedir(const char *path, struct fuse_file_info *fi)
{
	struct minix_inode *inode = (struct minix_inode *) fi->fh;

	debug("somix_releasedir(): releasing directory \"%s\"...", path);

	if(inode == NULL) {
		debug("release(\"%s\", ...): cannot release. "
			"file does not appear to be open", path);
		return -1;
	}

	put_inode(inode);


	return 0;
}

static int somix_statfs(const char *path, struct statvfs *svfs)
{
	debug("somix_statfs(): stat call made. path=\"%s\"", path);

	return 0;
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
	.write		= somix_write,
	.truncate	= somix_truncate,
	.unlink		= somix_unlink,
	.mkdir		= somix_mkdir,
	.rmdir		= somix_rmdir,
	.rename		= somix_rename,
	.releasedir	= somix_releasedir,
	.statfs		= somix_statfs,
/*
	.opendir	= minix_open,
	.mkdir		= minix_mkdir,
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
