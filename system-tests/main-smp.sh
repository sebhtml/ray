#!/bin/bash

for i in $(ls tests)
do
	./run-test.sh $i
done &> Report-SMP.txt
