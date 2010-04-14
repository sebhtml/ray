source ../0mix11-parameters.sh
source ../0parameters.sh
cat $p1left $p1right $p2left $p2right > reads.fastq
qualityTrimmer  -fastq reads.fastq -outFasta reads.fasta -type sanger 
Assemble.pl reads.fasta $wordSize 
ln -s $ref Reference.fasta
ln -s reads.fasta.contig Assembly.fasta
echo EULER-SR>Assembler.txt

