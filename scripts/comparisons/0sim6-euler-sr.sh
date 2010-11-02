source ../0sim5-parameters.sh
source ../0parameters.sh
cat $left $right > reads.fasta
Assemble.pl reads.fasta $wordSize 
ln -s $ref Reference.fasta
ln -s reads.fasta.contig Assembly.fasta
echo EULER-SR>Assembler.txt
