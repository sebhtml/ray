
source ../0parameters.sh
source ../0mix2-parameters.sh

time (rm -rf velvet
velveth velvet $wordSize -short -fastq $file1illumina -short -fastq $file2illumina &> log1
velvetg velvet -exp_cov 51 -cov_cutoff 10 &>log2)
print-latex.sh $adp1 velvet/contigs.fa Velvet

