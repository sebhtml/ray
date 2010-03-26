#!/bin/bash

if test $# -eq 0
then
	echo "usage"
	echo "print-latex.sh reference assembly assembler"
	exit
fi

assembler=$3
reference=$1
assembly=$2
assembly500=$assembly.500.fa

filter-contigs.py $assembly 500 $assembly500
mummer-validate.rb $reference  $assembly500 mums &> /dev/null
numberOfContigs=$(getlengths $assembly500|wc -l)
bases=$(getlengths $assembly500|awk '{sum+= $2} END {print sum}')
meanSize=$(getN50 $assembly500|head -n2|tail -n1|awk '{print $2}'| sed 's/\..*//')
n50=$(getN50 $assembly500|head -n3|tail -n1|awk '{print $2}')
max=$(getlengths $assembly500|awk '{print $2}'|sort -n|tail -n1)
coverage=$(printf %2.2f $(grep Coverage= mums|sed 's/Coverage=//'))
misassembled=$(grep Misas mums|awk '{print $3}')


extract-error-rates.py $reference $assembly500 > newMetrics.txt
mismatches=$(cat newMetrics.txt|grep totalMismatches=|sed 's/totalMismatches=//')
gaps=$(cat newMetrics.txt|grep totalGaps=|sed 's/totalGaps=//')
echo "        %  & numberOfContigs & bases & meanSize  & n50  & max   & coverage   & misassembled & mismatches & indels"
echo " $assembler & $numberOfContigs & $bases & $meanSize & $n50 &  $max &  $coverage & $misassembled & $mismatches & $gaps \\\\"
