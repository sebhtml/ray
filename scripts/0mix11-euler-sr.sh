
source ../0mix11-parameters.sh
source ../0parameters.sh
time (cat $p1left $p1right $p2left $p2right > reads.fastq
qualityTrimmer  -fastq reads.fastq -outFasta reads.fasta -type sanger &> qualityTrimmer.log
Assemble.pl reads.fasta $wordSize &> Assemble.pl.log ) 
print-latex.sh $mg1655 reads.fasta.contig EULER-SR

