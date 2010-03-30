source ../0parameters.sh
source ../0mix31-parameters.sh
velveth velvet $wordSize -fasta -short $r1 $r2 $r3 &> log1
velvetg velvet -exp_cov auto -cov_cutoff auto &> log2
print-latex.sh $ref velvet/contigs.fa Velvet
