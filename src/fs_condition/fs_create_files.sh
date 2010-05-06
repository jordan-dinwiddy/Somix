#!/bin/bash

# fs_create_file.sh - Create a number of randomly sized files in the 
# current working directory.
#
# The script will continue creating files until it has written greater
# than WRITE_LIMIT bytes of data.


SMALLEST=64			# The smallest file to write
LARGEST=256			# The largest file to write
WRITE_LIMIT=$((20*1024))	# Write this much in total

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

