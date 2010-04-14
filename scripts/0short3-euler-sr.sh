source ../0parameters.sh
source ../0short3-parameters.sh
qualityTrimmer -fastq $reads -outFasta reads.fasta -type sanger 
Assemble.pl reads.fasta $wordSize 
ln -s $ref Reference.fasta
ln -s reads.fasta.contig Assembly.fasta
echo EULER-SR > Assembler.txt
