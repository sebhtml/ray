#!/bin/bash

if test $# -eq 0
then
	echo "usage"
	echo "ValidateGenomeAssembly.sh reference assembly assembler"
	exit
fi

assembler=$3
reference=$1
assembly=$2
minimumLength=$4

prefix=$(echo $assembly|sed 's/Contigs.fasta//g')

assembly500=$assembly.$minimumLength.fa
mummerFile=$assembly.$minimumLength.mums

filter-contigs.py $assembly $minimumLength $assembly500

mummer-validate.rb $reference  $assembly500 $mummerFile &> mummer-validate.rb.log


scaffoldsFile=$prefix"Scaffolds.fasta"
scaffolds500=$scaffoldsFile".$minimumLength.fasta"
ValidateScaffolds.py $prefix > $prefix"ScaffoldValidation.txt"
numberOfIncorrectScaffolds=$(cat $prefix"ScaffoldValidation.txt"|tail -n1)
filter-contigs.py $scaffoldsFile $minimumLength $scaffolds500
numberOfScaffolds=$(grep '>' $scaffolds500|wc -l)

numberOfContigs=$(grep '>' $assembly500|wc -l)
bases=$(getlengths $assembly500|awk '{sum+= $2} END {print sum}')
meanSize=$(getN50 $assembly500|head -n2|tail -n1|awk '{print $2}'| sed 's/\..*//')
n50=$(getN50 $assembly500|head -n3|tail -n1|awk '{print $2}')
max=$(getlengths $assembly500|awk '{print $2}'|sort -n|tail -n1)
coverage=$(printf %2.4f $(grep Coverage= $mummerFile|sed 's/Coverage=//'))
misassembled=$(grep Misas $mummerFile|awk '{print $3}')

mismatches=$(grep totalMismatches $mummerFile|sed 's/totalMismatches=//g')
gaps=$(cat $mummerFile|grep totalGaps=|sed 's/totalGaps=//')

echo "        %  & numberOfContigs &scaffolds & bases & meanSize  & n50  & max   & coverage   & misassembledContigs & misassembledScaffolds & mismatches & indels"
echo " $assembler & $numberOfContigs & $numberOfScaffolds & $bases & $meanSize & $n50 &  $max &  $coverage & $misassembled & $numberOfIncorrectScaffolds & $mismatches & $gaps \\\\"
