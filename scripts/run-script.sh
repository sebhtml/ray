#!/bin/bash

i=$1
prefix=$(pwd)
echo "Running $i"
if test -f $i.res
then
	echo "$i is already done. ($i.res)"
elif test -d $i.dir
then
	echo "$i is already running. ($i.dir)"
else
	echo "Starting $i."
	mkdir $i.dir -p
	cd $i.dir
	(time bash ../$i > Log) &> WallClockTime
	cd ..
	analyseQuality.sh $i

	echo "$i is done."
	cd ..
fi

