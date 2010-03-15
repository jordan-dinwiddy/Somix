#ifndef _MINIX_PATH
#define _MINIX_PATH

#include <stdlib.h>		/* NULL type def */
#include "inode.h"


#define PATH_RESOLVE_ALL -1	/* used by path_resolve to resolve entire path */

int path_get_last_cmpo(const char *path, char *buf);
int path_cnt_cmpos(const char *path);
struct minix_inode *resolve_path(struct minix_inode *inode, const char *path, int n);

#endif
