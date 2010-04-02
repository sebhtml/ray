source ../0parameters.sh
source ../0sim8-parameters.sh
velveth velvet $wordSize -short -fasta $left $right &> log1
velvetg velvet -exp_cov auto -cov_cutoff auto &> log2
print-latex.sh $ref velvet/contigs.fa Velvet
