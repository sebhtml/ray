##!/bin/bash

# will be replaced upstream.
#$ -N __TEST_NAME__
# this is the project identifier
#$ -P nne-790-ab
# bound the execution  -- most system tests *will* finish under minutes
#$ -l h_rt=24:00:00
# utilize a number of compute cores in the compute grid -- also known as the mighty cloud
#$ -pe default __NSLOTS__

# load modules
module load compilers/gcc/4.4.2 mpi/openmpi/1.4.3_gcc

# will be replaced upstream.
TEST_NAME=__TEST_NAME__
RAY_GIT_PATH=__RAY_GIT_PATH__

echo ""
echo "BEGIN $TEST_NAME $(date)"
echo ""

cd $RAY_GIT_PATH/system-tests/tests/$TEST_NAME
export PATH=$PATH:$HOME/software/MUMmer3.22:$HOME/software/amos-2.0.8/build/bin:$RAY_GIT_PATH/scripts

echo "Changing current directory as it should be."

pwd

echo "Changed directory successfully -- system test $TEST_NAME will begin..."
echo ""

# remove old files

rm -rf $TEST_NAME

# run the system test
# actually, the system test can be executed on a standardd computer
# by exporting the variables TEST_NAME, RAY_GIT_PATH and NSLOTS
# and by then calling the said script.
source ./main.sh

echo ""
echo "THE_END $TEST_NAME $(date)"
echo ""
echo "Thank you, have a nice day."
echo ""
