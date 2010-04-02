source ../0parameters.sh
source ../0sim8-parameters.sh
shuffleSequences_fasta.pl $left $right reads.fasta
velveth velvet $wordSize -fasta -shortPaired reads.fasta &> log1
velvetg velvet -ins_length $length -exp_cov auto -cov_cutoff auto &> log2
print-latex.sh $ref velvet/contigs.fa Velvet
