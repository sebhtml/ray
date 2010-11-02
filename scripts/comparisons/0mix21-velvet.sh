source ../0parameters.sh
source ../0mix2-parameters.sh
velveth velvet $wordSize -short -fastq $file1illumina -short -fastq $file2illumina 
velvetg velvet -exp_cov 51 -cov_cutoff 10
ln -s velvet/contigs.fa Assembly.fasta
ln -s $ref Reference.fasta
echo Velvet>Assembler.txt
