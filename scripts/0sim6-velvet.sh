source ../0sim5-parameters.sh
source ../0parameters.sh

rm -rf velvet
velveth velvet $wordSize -short -fasta $left $right &> log1
velvetg velvet -exp_cov auto -cov_cutoff auto &> log2
print-latex.sh $ref velvet/contigs.fa Velvet
