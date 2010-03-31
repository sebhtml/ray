source ../0parameters.sh
source ../0short1-parameters.sh
cat $left $right > reads.fastq
qualityTrimmer  -fastq reads.fastq -outFasta reads.fasta -type sanger &>log1
Assemble.pl reads.fasta $wordSize  &>log2
print-latex.sh $syrin  reads.fasta.contig EULER-SR
