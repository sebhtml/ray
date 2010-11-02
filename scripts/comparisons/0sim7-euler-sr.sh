source ../0parameters.sh
source ../0sim8-parameters.sh
cat $left $right > reads.fasta
Assemble.pl reads.fasta $wordSize 
ln -s reads.fasta.contig Assembly.fasta
echo EULER-SR > Assembler.txt
ln -s $ref Reference.fasta
