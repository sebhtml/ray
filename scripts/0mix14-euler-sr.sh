source ../0parameters.sh
source ../0mix11-parameters.sh
source ../0mix13-parameters.sh
cat $p1left $p1right $p2left $p2right > reads.fastq
qualityTrimmer  -fastq reads.fastq -outFasta reads.1 -type sanger &> log1
cat reads.1 $r4541 $r4542 $r4543 > reads.fasta
regularExpression1="([^/]*s_4[^/]*)/([12])"
regularExpression2="([^/]*s_7[^/]*)/([12])"
echo "$regularExpression1 CloneLength=$length1 CloneVar=$sd1 Type=1      # Illumina mate-pair
$regularExpression2 CloneLength=$length2 CloneVar=$sd2 Type=1 " > mates.info
Assemble.pl reads.fasta $wordSize -ruleFile mates.info &> log2
print-latex.sh $ref reads.fasta.contig EULER-SR
