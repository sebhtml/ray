. 0parameters.sh
. 0short1-parameters.sh


time(
rm -rf velvet
shuffleSequences_fastq.pl $left $right reads.fastq
velveth velvet 21 -fastq -shortPaired reads.fastq &>/dev/null
velvetg velvet -ins_length 400 -cov_cutoff 7 -exp_cov 13  &>/dev/null
)
print-latex.sh $syrin velvet/contigs.fa Velvet
