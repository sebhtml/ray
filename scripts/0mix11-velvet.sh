source ../0mix11-parameters.sh
source ../0parameters.sh
velveth velvet $wordSize -short -fastq  $p1left $p1right $p2left $p2right &> log1
velvetg velvet -exp_cov 109 -cov_cutoff 10 &> log2
print-latex.sh $mg1655 velvet/contigs.fa Velvet


