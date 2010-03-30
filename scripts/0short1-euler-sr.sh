. 0parameters.sh
. 0short1-parameters.sh

time(
(renameReads-for-euler.py "" $left ; renameReads-for-euler.py "" $right) > reads.fq
qualityTrimmer  -fastq reads.fq -outFasta reads.fasta -type sanger &>/dev/null
echo '"([^/]*)/([12])" CloneLength=400 CloneVar=40 Type=1      # Illumina mate-pair' > mates.info
Assemble.pl reads.fasta $wordSize -ruleFile mates.info &>/dev/null )
print-latex.sh $syrin  Euler-reads.fasta.contig EULER-SR

