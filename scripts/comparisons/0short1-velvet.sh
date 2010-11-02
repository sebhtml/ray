source ../0parameters.sh
source ../0short1-parameters.sh
shuffleSequences_fastq.pl $left $right reads.fastq
velveth velvet $wordSize -fastq -shortPaired reads.fastq 
velvetg velvet -ins_length $length -cov_cutoff 7 -exp_cov 13 
ln -s velvet/contigs.fa Assembly.fasta
echo Velvet > Assembler.txt
ln -s $syrin Reference.fasta
