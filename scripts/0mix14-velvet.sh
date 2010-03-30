#!/bin/bash

source ../0mix11-parameters.sh
source ../0mix13-parameters.sh
source ../0parameters.sh

rm -rf velvet
velveth velvet $wordSize -short -fastq  $p1left $p1right $p2left $p2right -long -fasta $r4541 $r4542 $r4543 &> log1
velvetg velvet -exp_cov auto -cov_cutoff auto &> log2
print-latex.sh $ref velvet/contigs.fa Velvet
