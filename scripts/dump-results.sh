#!/bin/bash
for i in $(cat sets.txt|grep 'o '|awk '{print $2}')
do
	echo "========"
	grep $i sets.txt
	echo "%  & numberOfContigs & bases & meanSize  & n50  & max   & coverage   & misassembled & mismatches & indels"
	for j in newbler euler-sr velvet ray abyss
	do
		file=0$i-$j.sh.res
		if test -f $file
		then
			cat $file|grep -v applicable|grep -v numberOf
		else
			echo "MISSING RESULTS ($j)"
		fi
	done
done 

