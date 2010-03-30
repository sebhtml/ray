
. 0parameters.sh
. 0mix2-parameters.sh


time (cat $file1illumina $file2illumina > reads.fastq
qualityTrimmer  -fastq reads.fastq -outFasta reads.fasta -type illumina &>/dev/null
Assemble.pl reads.fasta $wordSize &>/dev/null) 
print-latex.sh $adp1 Euler-reads.fasta.contig EULER-SR

