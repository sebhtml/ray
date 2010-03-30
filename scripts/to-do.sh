#!/bin/bash
for i in $(cat sets.txt|grep 'o '|awk '{print $2}')
do
	for j in newbler euler-sr ray velvet abyss
	do
		file="0$i-$j.sh"
		if test -f $file
		then
			true
		else
			echo $file
		fi

	done
done > tmp

head tmp -n5
echo "($(($(cat tmp|wc -l)-5)) additional entries)"
