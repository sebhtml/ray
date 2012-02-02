#!/bin/bash

(

echo "Nothing reported = all unit tests passed"
echo ""

for i in $(ls *.sh|grep -v main.sh)
do
	echo "Test Suite: $i"
	bash $i
done 

) &> UnitTests.txt

echo "Wrote results in UnitTests.txt"
