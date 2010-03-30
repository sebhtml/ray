source ../0parameters.sh
source ../0mix13-parameters.sh

rm -rf velvet
velveth velvet $wordSize -long -fasta $r4541 $r4542 $r4543 &> log1
velvetg velvet -exp_cov auto -cov_cutoff auto &> log2
print-latex.sh $ref velvet/contigs.fa Velvet
