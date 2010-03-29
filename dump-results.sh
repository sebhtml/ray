#!/bin/bash

for i in $(ls *.Descriptors)
do 
	echo TestEntry: $i|sed 's/.Descriptors//g'
	grep Descr $i 
	cat $(echo $i|sed 's/Descriptors/Validation/g')
	echo ""
done

