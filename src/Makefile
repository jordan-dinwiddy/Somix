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

cache.o : cache.h comms.h const.h cache.c
	$(CC) -Wall -c cache.c

error.o : comms.h comms.c
	$(CC) -Wall -c comms.c

clean :
	rm *.o mf tool_print_bitmap tool_print_block tool_print_inode \
		tool_print_superblock
