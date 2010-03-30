#!/bin/bash

echo "Entry	Finished?	Checked?	Result	Directory"
for i in $(ls 0*.sh|grep -v parame)
do
	finished=No
	checked=No	
	directory=absent
	if test -f $i.check
	then
		checked=Yes
	fi
	if test -f $i.res
	then
		len=$(ls -l $i.res|awk '{print $5}') 
		if test $len -gt 0
		then
			finished=Yes
			if test -d $i.dir
			then
				directory=$i.dir
			fi
		fi
	fi

	echo "$i	$finished	$checked	$i.res	$directory"
done
