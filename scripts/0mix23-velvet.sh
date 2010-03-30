source ../0parameters.sh
source ../0mix2-parameters.sh
rm -rf velvet
velveth velvet $wordSize -short -fastq $file1illumina $file2illumina -long -fastq $r4541 $r4542 $r4543 $r4544 &> log1
velvetg velvet -exp_cov auto -cov_cutoff auto &> log2
print-latex.sh $ref velvet/contigs.fa Velvet
