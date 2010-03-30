#!/bin/bash

source ../0mix11-parameters.sh
source ../0parameters.sh


time(
rm -rf velvet
shuffleSequences_fastq.pl $p1left $p1right 1
shuffleSequences_fastq.pl $p2left $p2right 2
cat 1 2 > reads.fastq
velveth velvet 21 -fastq -shortPaired reads.fastq &>/dev/null
velvetg velvet -ins_length 215 -ins_length_sd 20 -exp_cov 109  -cov_cutoff 10 &>/dev/null
)
print-latex.sh $mg1655 velvet/contigs.fa Velvet

