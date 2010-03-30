
. 0mix11-parameters.sh
. 0parameters.sh
time (cat $p1left $p1right $p2left $p2right > reads.fastq
qualityTrimmer  -fastq reads.fastq -outFasta reads.fasta -type sanger &>/dev/null
Assemble.pl reads.fasta $wordSize &>/dev/null) 
print-latex.sh $mg1655 reads.fasta.contig EULER-SR

