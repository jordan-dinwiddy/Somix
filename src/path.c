#include <stdlib.h>
#include "types.h"
#include "const.h"
#include "inode.h"
#include "read.h"
#include "comms.h"
#include "superblock.h"
#include "path.h"

extern struct minix_super_block sb;

/**
 * Copies the n'th component of the given 'path' to buf. The component will be
 * stripped of preceeding anid trailing slashes.
 *
 * Components are indexed starting at zero.
 *
 * Returns 1 if the n'th component exists and was copied succesfully, 0
 * otherwise.
 */
static int path_get_nth_cmpo(const char *path, char *buf, int n)
{
	int offset = 0;				
	
	while(*(path + offset) == '/') 
		offset++;		/* skip any leading slashes */

	/* skip past first n components in path. After skipping past n
	 * components we will be left pointing to the start of the
	 * n+1'th component exluding its leading slash. */
	for( ; *(path+offset) != '\0' && n != 0; ) {
		if(*(path+offset) == '/') {
			/* allows //foo////bar etc. */
			while(*(path+offset)=='/') offset++; 

			n--;
			continue;
		}

		offset++;
	}

	if(n != 0) return 0;		/* n'th component doesn't exist */

	if(*(path+offset) == '\0') return 0;	/* We had a trailing slash */
	while(*(path+offset) != '/' && *(path+offset) != '\0') { 
		*buf = *(path+offset);
		offset++;
		buf++;
	}
	*buf = '\0';

	return 1;
}

/**
 * Counts the number of components in the given path.
 *
 * Examples:
 * 	/ 		- 0 components
 *	//		- 0 components
 *	/home		- 1 component
 *	/home/foo/bar/ 	- 3 components
 *
 * Returns the number of components
 */
int path_cnt_cmpos(const char *path)
{
	int i = 0;
	while(*path != '\0') {
		/* eat slashes */
		while(*path == '/' && *path != '\0') 
			path++;

		if(*path != '\0') i++;
		
		/* eat component */
		while(*path != '/' && *path != '\0') 
			path++; 
 			
	}

	return i;
}

/**
 * Copies the last component of the given path into 'buf'.
 *
 * Returns 1 on success, 0 otherwise when the last component cannot be retrieve
 * because the given path has 0 components. See 'path_cnt_cmpos'.
 */
int path_get_last_cmpo(const char *path, char *buf)
{
	int cnt = 0;
	if((cnt = path_cnt_cmpos(path)) > 0) {
		path_get_nth_cmpo(path, buf, cnt-1);
		return 1;
	}

	return 0;
}

/**
 * Given a directory inode and a string path relative to that inode this
 * function resolves the path up to the n'th component and returns the inode.
 *
 * For the path /home/jo/foo
 * 	resolve_path(/home/jo/foo, 0) 	- returns 'inode'
 * 	resolve_path(/home/jo/foo, 1)  	- returns inode for home
 * 	resolve_path(/home/jo/foo, 2)	- returns inode for jo
 * 	etc.
 *
 * set n to PATH_RESOLVE_ALL to fully resolve the path.
 *
 * The inode given is never 'put'
 *
 * Returns the inode if the path is valid, NULL otherwise.
 */
#ifdef _FOO
struct minix_inode *resolve_path(struct minix_inode *inode, const char *path, 
	int n)
{
	char file[FILENAME_SIZE];
	int i = 0;
	inode_nr i_num;

	debug("resolve_path(inode %d, \"%s\", %d): resolving %d'th "
		"component...", inode->i_num, path, n, n);

	while(n != 0 && path_get_nth_cmpo(path, file, i)) {
		debug("resolv_path(...): looking up component \"%s\" in "
			"inode %d...", file, inode->i_num);
		i_num = dir_search(inode, file);

		if(i_num == NO_INODE) {
			debug("resolv_path(...): unable to find component "
			"\"%s\"", file);
			return NULL;
		}
		debug("resolv_path(...): component \"%s\" found with inode=%d",
			file, i_num);

		/* only put inodes we got ourselves */
		if(i++ > 0) put_inode(inode); 	
		inode = get_inode(i_num);

		n--;			
	}
	
	if(n > 0) {
		/* we ran out of components before getting to n'th */
		debug("resolv_path(...): not enough components in path to "
			"return n'th");
		if(i > 0) put_inode(inode);
		inode = NULL;
	}

	/* so the caller is free to put the inode they resolved */
	if(i == 0) inode = get_inode(inode->i_num);

	debug("returning inode %d", inode->i_num);
	return inode;
}
#endif

struct minix_inode *resolve_path(struct minix_inode *root, const char *path, 
	int levels)
{
	char file[FILENAME_SIZE];
	int i;
	struct minix_inode *inode;
	inode_nr i_num;

	if(levels == PATH_RESOLVE_ALL)
		levels = path_cnt_cmpos(path);

	/* NOTE: caller must always be free to put the inode we return */
	if(levels == 0)
		return get_inode(root->i_num);

	/* load a copy of root inode to start with */
	inode = get_inode(root->i_num);

	for(i = 0; i < levels; i++) {
		if(!path_get_nth_cmpo(path, file, i)) {
			/* couldn't get component, shouldn't happen since we
			 * counted components first. */
			panic("resolve_path(): couldn't get component %d", i);
		}

		i_num = dir_search(inode, file);
		if(i_num == NO_INODE) {
			debug("resolve_path(): failed to lookup \"%s\" in "
				"inode %d", file, inode->i_num);
			put_inode(inode);
			return NULL;
		}

		put_inode(inode);
		inode = get_inode(i_num);
	}

	debug("resolve_path(): finished resolving path, returning inode %d",
		inode->i_num);
	return inode;
}
		
	
/**
 * Returns the inode corresponding to the final directory in the given path.
 * Also returns the final component of the path in 'buf'.
 * return NULL if error
 */
struct minix_inode *last_dir(const char *path, char *buf)
{
	int n = path_cnt_cmpos(path);
	struct minix_inode *p_dir;

	if(path_get_last_cmpo(path, buf) == 0) 
		return NULL;	/* 0 components in path = no file specified */

	p_dir = resolve_path(sb.root_inode, path, n - 1);

	return p_dir;
}


/**
 * looks up the given file ine parent direct, opens the inode and returns it.
 *
 * if file cannot be found in directory specified returns NULL.
 */
struct minix_inode *advance(struct minix_inode *p_dir, const char *filename)
{	
	inode_nr i_num;
	struct minix_inode *retval;

	debug("advance(%d, \"%s\"):", p_dir->i_num, filename);

	if((i_num = dir_search(p_dir, filename)) == NO_INODE) {
		debug("advance(%d, \"%s\"): couldn't find file in directory",
			p_dir->i_num, filename);
		return NULL;
	}

	retval = get_inode(i_num);
	
	return retval;
}

	
