
source ../0mix11-parameters.sh
source ../0parameters.sh

time(
(renameReads-for-euler.py /1 $p1left ;renameReads-for-euler.py /2 $p1right ;renameReads-for-euler.py /1 $p2left ; renameReads-for-euler.py /2 $p2right) > reads.fq
qualityTrimmer  -fastq reads.fq -outFasta reads.fasta -type sanger &>/dev/null
echo '"([^/]*)/([12])" CloneLength=215 CloneVar=20 Type=1      # Illumina mate-pair' > mates.info
Assemble.pl reads.fasta $wordSize -ruleFile mates.info &>/dev/null )
print-latex.sh $mg1655 Euler-reads.fasta.contig EULER-SR


