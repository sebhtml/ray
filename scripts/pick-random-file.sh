#!/bin/bash

# pick up a random source file

find ~/git-clones/ray -name "*.cpp" > list
find ~/git-clones/ray -name "*.h" >> list
count=$(cat list|wc -l)
randomNumber=$RANDOM
fileNumber=$(($randomNumber % $count))
fileNumberStartingAt1=$(($fileNumber+1))
file=$(head -n $fileNumberStartingAt1 list|tail -n1)

echo $file
