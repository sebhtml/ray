source ../0parameters.sh
source ../0short1-parameters.sh


time(
rm -rf velvet
shuffleSequences_fastq.pl $left $right reads.fastq
velveth velvet $wordSize -fastq -shortPaired reads.fastq &> log1
velvetg velvet -ins_length $length -cov_cutoff 7 -exp_cov 13  &>log2
)
print-latex.sh $syrin velvet/contigs.fa Velvet
