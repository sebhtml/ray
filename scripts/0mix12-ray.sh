#!/bin/bash

source ../0mix11-parameters.sh
source ../0parameters.sh


time mpirun -np $nproc Ray -p $p1left $p1right $length1 $sd1 -p $p2left $p2right $length2 $sd2 &> log1
print-latex.sh $mg1655 Ray-Contigs.fasta Ray
