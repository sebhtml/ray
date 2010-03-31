source ../0parameters.sh
source ../0short3-parameters.sh
qualityTrimmer -fastq $reads -outFasta reads.fasta -type sanger &> log1
Assemble.pl reads.fasta $wordSize &> log2
print-latex.sh $ref reads.fasta.contig EULER-SR
