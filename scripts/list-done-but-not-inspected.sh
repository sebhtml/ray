#!/bin/bash

for i in $(ls 0*.sh|grep -v parame)
do
	if test -f $i.res
	then
		len=$(ls -l $i.res|awk '{print $5}') 
		if test $len -gt 0
		then
			if test -d $i.dir
			then
				echo "Check $i.dir"
			fi
		fi
	fi
done
