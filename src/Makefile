objects = minix_fuse.o minix_fuse_lib.o
CC = gcc -O2

all : somix mkfs.somix tests 

tests: test_cache test_resolv_path 
	
test_cache : test_cache.c cache.o comms.o const.h
	$(CC) -Wall test_cache.c cache.o comms.o -o test_cache

test_resolv_path : test_resolv_path.c comms.o bitmap.o mount.o cache.o \
		inode.o path.o read.o
	$(CC) -Wall test_resolv_path.c comms.o bitmap.o mount.o cache.o \
		inode.o path.o read.o -o test_resolv_path

somix : somix.c comms.o bitmap.o mount.o cache.o inode.o path.o read.o write.o
	$(CC) -Wall -D_FILE_OFFSET_BITS=64 -I/usr/local/include/fuse \
		-pthread -L/usr/local/lib -lfuse -lrt -ldl \
		 somix.c comms.o bitmap.o mount.o cache.o inode.o path.o \
		read.o write.o -o somix

mkfs.somix : include/nls.h include/minix.h include/bitops.h mkfs.somix.c
	$(CC) -Iinclude -fsigned-char -fomit-frame-pointer -O3 -o \
		mkfs.somix mkfs.somix.c

path.o : path.c path.h types.h const.h inode.h read.h comms.h
	$(CC) -Wall -c path.c

inode.o : inode.c inode.h superblock.h comms.h
	$(CC) -Wall -c inode.c

read.o : read.c read.h types.h const.h cache.h inode.h comms.h
	$(CC) -Wall -c read.c

write.o : write.c types.h superblock.h comms.h const.h cache.h inode.h write.h
	$(CC) -Wall -c write.c

comms.o : comms.c comms.h
	$(CC) -Wall -c comms.c

mount.o : mount.c mount.h const.h cache.o comms.o
	$(CC) -Wall -c mount.c

bitmap.o : bitmap.c bitmap.h const.h cache.o comms.o
	$(CC) -Wall -c bitmap.c

cache.o : cache.h comms.h const.h cache.c
	$(CC) -Wall -c cache.c

error.o : comms.h comms.c
	$(CC) -Wall -c comms.c

clean :
	rm *.o somix test_cache test_resolv_path mkfs.somix
