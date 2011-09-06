#!/bin/bash

# run a system test, but not on the compute grid
# the appropriate build must be available in builds/

TEST_NAME=$1

if [ -z $NSLOTS ]
then
	export NSLOTS=$(cat /proc/cpuinfo|grep proc|wc -l)
fi

RAY_GIT_PATH=~/git-clones/ray

echo "TEST_NAME= $TEST_NAME"
echo "NSLOTS= $NSLOTS"
echo "RAY_GIT_PATH= $RAY_GIT_PATH"
cd tests/$TEST_NAME

rm -rf $TEST_NAME

source ./main.sh
