#!/bin/bash

i=$1
prefix=$(pwd)
echo "Running $i"
if test -f $i.res
then
	echo "$i is already done."
elif test -f $i.dir
then
	echo "$i is already running."
else
	echo "Starting $i."
	mkdir $i.dir -p
	cd $i.dir
	bash ../$i  > ../$i.res
	echo "$i is done."
	cd ..
	rm -rf $i.dir
fi

