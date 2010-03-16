objects = minix_fuse.o minix_fuse_lib.o
CC = gcc

all : mf tool_print_bitmap tool_print_block tool_print_inode \
	tool_print_superblock
	
mf : minix_fuse_lib.o minix_fuse.o
	$(CC) -o mf -Wall -pthread -L/usr/local/lib -lfuse -lrt -ldl \
		$(objects)

tool_print_bitmap : minix_fuse_lib.o minix_fuse_lib.h tool_print_bitmap.c
	$(CC) minix_fuse_lib.o tool_print_bitmap.c -o tool_print_bitmap

tool_print_block : minix_fuse_lib.o minix_fuse_lib.h tool_print_block.c
	$(CC) minix_fuse_lib.o tool_print_block.c -o tool_print_block

tool_print_inode : minix_fuse_lib.o minix_fuse_lib.h tool_print_inode.c
	$(CC) minix_fuse_lib.o tool_print_inode.c -o tool_print_inode

tool_print_superblock : minix_fuse_lib.o minix_fuse_lib.h \
	tool_print_superblock.c
	$(CC) minix_fuse_lib.o tool_print_superblock.c -o \
		tool_print_superblock

minix_fuse.o : minix_fuse_lib.h minix_fuse.c
	$(CC) -Wall -D_FILE_OFFSET_BITS=64 -I/usr/local/include/fuse \
		-c minix_fuse.c

minix_fuse_lib.o : minix_fuse_lib.h minix_fuse_lib.c
	$(CC) -Wall -c minix_fuse_lib.c

cache_test : cache_test.c cache.o comms.o const.h
	$(CC) -Wall cache_test.c cache.o comms.o -o cache_test

test_mount : test_mount.c mount.o cache.o comms.o bitmap.o
	$(CC) -Wall test_mount.c mount.o cache.o comms.o bitmap.o -o test_mount

test_resolv_path : test_resolv_path.c comms.o bitmap.o mount.o cache.o \
		inode.o path.o read.o
	$(CC) -Wall test_resolv_path.c comms.o bitmap.o mount.o cache.o \
		inode.o path.o read.o -o test_resolv_path

somix : somix.c comms.o bitmap.o mount.o cache.o inode.o path.o read.o write.o
	$(CC) -Wall -D_FILE_OFFSET_BITS=64 -I/usr/local/include/fuse \
		-pthread -L/usr/local/lib -lfuse -lrt -ldl \
		 somix.c comms.o bitmap.o mount.o cache.o inode.o path.o \
		read.o write.o -o somix

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
	rm *.o mf tool_print_bitmap tool_print_block tool_print_inode \
		tool_print_superblock
