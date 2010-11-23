#!/bin/bash
#$ -N r3848
#$ -P -------
#$ -l h_rt=0:20:00
#$ -pe mpi 32
module load compilers/gcc/4.4.2 mpi/openmpi/1.3.4_gcc
/software/MPI/openmpi-1.3.4_gcc/bin/mpirun /home/sboi/nne-790-aa/pseudo-ticket-581/r3848/code/Ray \
-p /home/sboi/nne-790-aa/pseudo-ticket-581/Pseud,200b,2x50b,50X_1.fasta \
   /home/sboi/nne-790-aa/pseudo-ticket-581/Pseud,200b,2x50b,50X_2.fasta \
-o /home/sboi/nne-790-aa/pseudo-ticket-581/r3848/r3848.fasta

