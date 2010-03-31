source ../0parameters.sh
source ../0short1-parameters.sh
(renameReads-for-euler.py "" $left ; renameReads-for-euler.py "" $right) > reads.fq
qualityTrimmer  -fastq reads.fq -outFasta reads.fasta -type sanger &>log1
echo '"([^/]*)/([12])" CloneLength=400 CloneVar=40 Type=1      # Illumina mate-pair' > mates.info
Assemble.pl reads.fasta $wordSize -ruleFile mates.info &>log2
print-latex.sh $syrin  reads.fasta.contig EULER-SR

