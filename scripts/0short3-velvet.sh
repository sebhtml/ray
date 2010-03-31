source ../0parameters.sh
source ../0short3-parameters.sh
velveth velvet $wordSize -short -fastq $reads &> log1
velvetg velvet -cov_cutoff auto -exp_cov auto &> log2
print-latex.sh $ref velvet/contigs.fa Velvet
