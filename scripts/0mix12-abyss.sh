#!/bin/bash

source ../0mix11-parameters.sh
source ../0parameters.sh

time abyss-pe k=$wordSize n=10 name=mg1655 lib="lib1 lib2" lib1="$p1left $p1right" lib2="$p2left $p2right"  &>/dev/null
print-latex.sh  $mg1655 mg1655-contigs.fa ABySS

