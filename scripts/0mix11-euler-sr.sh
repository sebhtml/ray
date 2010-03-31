source ../0mix11-parameters.sh
source ../0parameters.sh
cat $p1left $p1right $p2left $p2right > reads.fastq
qualityTrimmer  -fastq reads.fastq -outFasta reads.fasta -type sanger &> log1
Assemble.pl reads.fasta $wordSize &> log2
print-latex.sh $mg1655 reads.fasta.contig EULER-SR

