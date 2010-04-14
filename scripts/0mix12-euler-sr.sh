source ../0mix11-parameters.sh
source ../0parameters.sh
(renameReads-for-euler.py /1 $p1left ;renameReads-for-euler.py /2 $p1right ;renameReads-for-euler.py /1 $p2left ; renameReads-for-euler.py /2 $p2right) > reads.fq
qualityTrimmer  -fastq reads.fq -outFasta reads.fasta -type sanger 
regularExpression1="([^/]*s_4[^/]*)/([12])"
regularExpression2="([^/]*s_7[^/]*)/([12])"
echo "$regularExpression1 CloneLength=$length1 CloneVar=$sd1 Type=1      # Illumina mate-pair
$regularExpression2 CloneLength=$length2 CloneVar=$sd2 Type=1 " > mates.info
Assemble.pl reads.fasta $wordSize -ruleFile mates.info 
ln -s $ref Reference.fasta
ln -s reads.fasta.contig Assembly.fasta
echo EULER-SR>Assembler.txt


