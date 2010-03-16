#include <stdlib.h>
#include "types.h"
#include "const.h"
#include "inode.h"
#include "read.h"
#include "comms.h"
#include "path.h"

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
		put_inode(inode);
		inode = NULL;
	}

	debug("returning inode %d", inode->i_num);
	return inode;
}
