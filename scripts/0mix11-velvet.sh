#!/bin/bash

source ../0mix11-parameters.sh
source ../0parameters.sh

time (rm -rf velvet
velveth velvet $wordSize -short -fastq  $p1left $p1right $p2left $p2right &>/dev/null
velvetg velvet -exp_cov 109 -cov_cutoff 10 &>/dev/null)
print-latex.sh $mg1655 velvet/contigs.fa Velvet


