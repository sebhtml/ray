source ../0parameters.sh
source ../0mix2-parameters.sh
cat $file1illumina $file2illumina > reads.fastq
qualityTrimmer  -fastq reads.fastq -outFasta reads.fasta -type sanger 
Assemble.pl reads.fasta $wordSize 
ln -s $ref Reference.fasta
ln -s reads.fasta.contig Assembly.fasta
echo EULER-SR>Assembler.txt
