source ../0parameters.sh
source ../0sim8-parameters.sh
cat $left $right > reads.fasta
echo '"([^/]*)_([12])" CloneLength=250 CloneVar=0 Type=1      # Illumina mate-pair' > mates.info
Assemble.pl reads.fasta $wordSize -ruleFile mates.info 
ln -s reads.fasta.contig Assembly.fasta
ln -s $ref Reference.fasta
echo EULER-SR > Assembler.txt
