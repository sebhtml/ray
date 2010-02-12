#!/bin/bash
#$ -N Chromosome1
#$ -P nne-790-aa
#$ -l h_rt=24:00:00
#$ -pe hosts 64
module load compilers/gcc/4.1.2 mpi/openmpi/1.4.1_gcc 
/software/MPI/openmpi-1.4.1_gcc/bin/mpirun /home/sboisver12/Ray/trunk/Ray
/home/sboisver12/nne-790-aa/Ray-input.txt

