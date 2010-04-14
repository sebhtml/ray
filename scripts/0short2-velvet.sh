source ../0parameters.sh
source ../0short1-parameters.sh
velveth velvet $wordSize -fastq -short $left $right 
velvetg velvet -cov_cutoff 7 -exp_cov 13  
echo Velvet > Assembler.txt
ln -s velvet/contigs.fa Assembly.fasta
ln -s $syrin Reference.fasta
