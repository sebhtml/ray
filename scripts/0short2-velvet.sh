source ../0parameters.sh
source ../0short1-parameters.sh
velveth velvet $wordSize -fastq -short $left $right &> log1
velvetg velvet -cov_cutoff 7 -exp_cov 13  &>log2
print-latex.sh $syrin velvet/contigs.fa Velvet
