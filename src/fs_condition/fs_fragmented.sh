#!/bin/bash

# fs_fragmented.sh - Attempts to fragment a filesystem by creating a 
# number of randomly sizes files in the current working directory and
# then deleting every other one. This results in serious fragmentation.
#
# The script will continue creating files until it has written greater
# than WRITE_LIMIT bytes of data.
#
# It's operation is very similar to fs_create_files.sh except it has
# the additional step of removing some files after first creating them.

SMALLEST=64
LARGEST=256
WRITE_LIMIT=$((60*1024))

# $0 = min
# $1 = max
function get_random {
	number=0   #initialize
	while [ "$number" -le $1 ]
	do
		number=$RANDOM
		let "number %= $2"  # Scales $number down within $RANGE.
	done
	
	return $number
}

echo "Creating files randomly sized between $SMALLEST and $LARGEST KB in" \
	"directory `pwd`..."
echo "Stopping when total KB written >= $WRITE_LIMIT."

i=0;
count=0;
this_size=0;
while [ "$i" -le $WRITE_LIMIT ]
do
	get_random $(($SMALLEST-1)) $(($LARGEST+1))
	this_size=$?
	dd if=/dev/zero bs=1k count=$this_size > dummy_$count 2> /dev/null
	count=$(($count+1))
	i=$(($i+$this_size))
done

echo "$count files successfully written"

echo "Deleting every other file..."

j=0
for((i=0; i<$count; i+=2))
do
	rm dummy_$i
	j=$(($j+1))
done

echo "$j files deleted"
echo "Fragmentation complete"


