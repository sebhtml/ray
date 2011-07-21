#!/bin/bash

# edit 4 files at random

for i in $(seq 4)
do
	vim $(scripts/pick-random-file.sh)
done
