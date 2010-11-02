source ../0parameters.sh
source ../0short1-parameters.sh
cat $left $right > reads.fastq
qualityTrimmer  -fastq reads.fastq -outFasta reads.fasta -type sanger 
Assemble.pl reads.fasta $wordSize 
echo EULER-SR > Assembler.txt
ln -s reads.fasta.contig Assembly.fasta
ln -s $syrin Reference.fasta
