source ../0parameters.sh
source ../0short1-parameters.sh
(renameReads-for-euler.py "" $left ; renameReads-for-euler.py "" $right) > reads.fq
qualityTrimmer  -fastq reads.fq -outFasta reads.fasta -type sanger 
echo '"([^/]*)/([12])" CloneLength=400 CloneVar=40 Type=1      # Illumina mate-pair' > mates.info
Assemble.pl reads.fasta $wordSize -ruleFile mates.info 
ln -s $syrin Reference.fasta
ln -s reads.fasta.contig Assembly.fasta
echo EULER-SR > Assembler.txt
