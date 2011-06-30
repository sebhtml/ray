##!/bin/bash

RayTestName=__TEST_NAME__

#$ -N $RayTestName
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
echo "BEGIN $RayTestName $(date)"
echo ""

cd ~/RayIntegrationTests/tests/$RayTestName

source main.sh

echo ""
echo "THE_END $RayTestName $(date)"
echo ""
