#!/bin/bash

TEST_NAME=$1
testName=$TEST_NAME

if [ -z $RAY_GIT_PATH ]
then
	export RAY_GIT_PATH=~/git-clones/ray
fi

if [ -z $NSLOTS ]
then
	export NSLOTS=32
fi

qsubFile=template.sh-qsub-$testName
cp template.sh $qsubFile
expression="s/__TEST_NAME__/$testName/g"
sed -i $expression $qsubFile

expression3="s/__NSLOTS__/$NSLOTS/g"
sed -i $expression3 $qsubFile

# @ is the delimiter here.
expression2="s@__RAY_GIT_PATH__@$RAY_GIT_PATH@g"
sed -i $expression2 $qsubFile
qsubOut=qsub-out-$testName
qsub $qsubFile 
