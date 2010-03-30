#!/bin/bash
for i in $(ls 0*.sh|grep -v parame)
do
	if test -f $i.res
	then
		true
	elif test -d $i.dir
	then
		true
	else
		echo $i
	fi
done
