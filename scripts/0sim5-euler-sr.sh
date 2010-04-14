source ../0parameters.sh
source ../0sim5-parameters.sh
cat $left $right > reads.fasta
echo '"([^/]*)_([12])" CloneLength=200 CloneVar=0 Type=1      # Illumina mate-pair' > mates.info
Assemble.pl reads.fasta $wordSize -ruleFile mates.info 
echo EULER-SR>Assembler.txt
ln -s $ref Reference.fasta
ln -s reads.fasta.contig Assembly.fasta
