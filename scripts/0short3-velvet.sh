source ../0parameters.sh
source ../0short3-parameters.sh
velveth velvet $wordSize -short -fastq $reads
velvetg velvet -cov_cutoff 4 -exp_cov 19
echo Velvet > Assembler.txt
ln -s velvet/contigs.fa Assembly.fasta
ln -s $ref Reference.fasta
