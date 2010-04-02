source ../0sim2-parameters.sh
source ../0parameters.sh
velveth velvet $wordSize -fasta -short $reads &> log1
velvetg velvet -cov_cutoff auto -exp_cov auto &> log2
print-latex.sh $ref  velvet/contigs.fa Velvet
