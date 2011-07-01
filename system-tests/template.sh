##!/bin/bash

source config

TEST_NAME=__TEST_NAME__

#$ -N $TEST_NAME
#$ -P nne-790-ab
#$ -l h_rt=2:00:00
#$ -pe default 32
# Set email address for notification
#$ -M seb@boisvert.info
# Email to be sent when the job starts and ends
#$ -m beas

# load modules
module load compilers/gcc/4.4.2 mpi/openmpi/1.4.3_gcc

echo ""
echo "BEGIN $TEST_NAME $(date)"
echo ""

cd $RAY_GIT_PATH/system-tests/tests/$TEST_NAME

source main.sh

echo ""
echo "THE_END $TEST_NAME $(date)"
echo ""
