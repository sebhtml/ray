source ../0parameters.sh
source ../0mix11-parameters.sh
source ../0mix13-parameters.sh


cat $p1left $p1right $p2left $p2right > reads.fastq
qualityTrimmer  -fastq reads.fastq -outFasta 1 -type sanger &> log1
cat 1 $r4541 $r4542 $r4543 > reads.fasta
Assemble.pl reads.fasta $wordSize &> log2
print-latex.sh $ref reads.fasta.contig EULER-SR
