source ../0parameters.sh
source ../0sim5-parameters.sh

rm -rf velvet
shuffleSequences_fasta.pl $left $right reads.fasta
velveth velvet $wordSize -fasta -shortPaired reads.fasta &> log1
velvetg velvet -ins_length $length -exp_cov auto -cov_cutoff auto &> log
print-latex.sh $ref velvet/contigs.fasta Velvet
