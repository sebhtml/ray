#!/bin/bash
for i in $(cat sets.txt|grep 'o '|awk '{print $2}')
do
	echo "========"
	grep $i sets.txt
	for j in newbler euler-sr velvet ray abyss
	do
		file=0$i-$j.sh.res
		if test -f $file
		then
			cat $file
		else
			echo "MISSING RESULTS ($j)"
		fi
	done
done 

