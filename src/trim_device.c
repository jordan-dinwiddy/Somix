/**
 * Attempts to issue TRIM commands to a specified target TRIM 
 * capable device.
 *
 * TRIM's are issued using IOCTL.
 *
 * Author: Jordan Dinwiddy
 * Date: March 2010
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <mntent.h>
#include <string.h>

#ifndef BLKGETSIZE
#define BLKGETSIZE _IO(0x12,96)    /* return device size */
#endif

/**
 * For ioctl discard/trim commands.
 */
#ifndef BLKDISCARD
#define BLKDISCARD _IO(0x12,119)    /* return device size */
#endif

int fd;

static unsigned long
get_size(void) {
	unsigned long	size;

	if (ioctl(fd, BLKGETSIZE, &size) >= 0) {
		return (size * 512);
	}
	
	perror("unable to ioctl block size\n");
	return size;
}

/*
 * Check to make certain that our new filesystem won't be created on
 * an already mounted partition.  Code adapted from mke2fs, Copyright
 * (C) 1994 Theodore Ts'o.  Also licensed under GPL.
 */
static int
is_mounted(const char *device) {
	FILE * f;
	struct mntent * mnt;

	if ((f = setmntent (MOUNTED, "r")) == NULL)
		return 0;
	while ((mnt = getmntent (f)) != NULL)
		if (strcmp (device, mnt->mnt_fsname) == 0)
			break;
	endmntent (f);
	if (!mnt)
		return 0;

	return 1;
}

int main(int argc, char **argv)
{
	unsigned long dev_size = 0;
	unsigned long range[2];
	int ret = -1;

	if(argc != 2) {
		printf("Usage: %s [device_file]\n", argv[0]);
		return -1;
	}

	printf("Trim Device Version 0.12\n");
	printf("Jordan Dinwiddy 2010\n\n");

	printf(" -device=%s\n", argv[1]);

	printf(" -is mounted... ");
	if(is_mounted(argv[1])) {
		printf("yes\n\nPlease unmount device\n");
		exit(1);
	}
	else {
		printf("no\n");
	}

	printf(" -opening... ");
	fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		printf("failed\n");
		exit(1);
	}
	else {
		printf("success, fd=%d\n", fd);
	}

	dev_size = get_size();
	printf(" -size=%ld bytes, %ld KB, %ld MB, %ld GB\n",
			dev_size, dev_size / 1024, dev_size / (1024 * 1024), 
			dev_size / (1024 * 1024 * 1024));

	range[0] = 0;
	range[1] = dev_size;
	
	printf(" -trimming bytes %ld -> %ld... ", range[0], range[1]);
	ret = ioctl(fd, BLKDISCARD, &range);

	if(ret) {
		printf("failed. errno=%d, reason=%s\n", errno, 
			strerror(errno));
	}
	else
		printf("success\n");

	printf(" -closing device...\n");
	close(fd);
		
	return ret;
}
	 
