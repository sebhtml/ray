#!/bin/bash
#$ -N Ray
#$ -P nne-790-aa
#$ -l h_rt=12:00:00
#$ -pe mpi 256
module load compilers/gcc/4.4.2 mpi/openmpi/1.3.4_gcc
/software/MPI/openmpi-1.3.4_gcc/bin/mpirun /home/sboisver12/Ray/tags/0.0.7/Ray /home/sboisver12/nne-790-aa/colosse.clumeq.ca/qsub/Ray-input.txt
Human-chr1-ompi-1.3.4-gcc.sh
