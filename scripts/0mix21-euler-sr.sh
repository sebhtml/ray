
source ../0parameters.sh
source ../0mix2-parameters.sh


time (cat $file1illumina $file2illumina > reads.fastq
qualityTrimmer  -fastq reads.fastq -outFasta reads.fasta -type sanger &>log1
Assemble.pl reads.fasta $wordSize &>log2)
print-latex.sh $adp1 Euler-reads.fasta.contig EULER-SR

