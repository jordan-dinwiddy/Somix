#!/bin/bash

# This script will make NUM_WRITES small writes to append 1K of data each time
# randomly to files within the current directory.

NUM_WRITES=6000		# Make 6K individiual writes.
			# Chosen this number because when fragmenting FS with
			# fs_fragmented.sh and SMALLEST=32 and LARGEST=64
			# and WRITE_LIMIT=20Meg, we usually get about 420
			# files created, 210 deleted. If each file is a min
			# of 32K large then there should be 210*32K worth
			# of fragmented free space = 6720K.

echo "Randomly appending data to files in `pwd` ..."

file_matrix=($(ls `pwd`))
num_files=${#file_matrix[*]}

echo "There are $num_files files in this directory."

echo "Performing $NUM_WRITES random append-style writes to files in" \
	"directory..."

for((i=0; i<$NUM_WRITES; i++))
do
	file=${file_matrix[$((RANDOM%num_files))]}
	dd if=/dev/zero bs=1K count=1 >> $file 2>/dev/null
done

echo "All random writes complete"
