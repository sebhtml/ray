#!/bin/bash
#$ -N Ticket557
#$ -P nne-790-aa
#$ -l h_rt=0:40:00
#$ -pe mpi 128
#$ -M sebastien.boisvert.3@ulaval.ca
#$ -m bea
module load compilers/gcc/4.4.2 mpi/openmpi/1.4.3_gcc
/software/MPI/openmpi-1.4.3_gcc/bin/mpirun /home/sboisver12/Ray/trunk/code/Ray  \
-p /home/sboisver12/nne-790-aa/SRA001125/SRR001665_1.fastq.gz /home/sboisver12/nne-790-aa/SRA001125/SRR001665_2.fastq.gz \
-p /home/sboisver12/nne-790-aa/SRA001125/SRR001666_1.fastq.gz /home/sboisver12/nne-790-aa/SRA001125/SRR001666_2.fastq.gz \
-o Ecoli-THEONE

