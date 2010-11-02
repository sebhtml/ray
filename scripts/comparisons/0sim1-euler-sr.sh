source ../0sim1-parameters.sh
source ../0parameters.sh
cp $reads reads.fasta
Assemble.pl reads.fasta $wordSize 
ln -s reads.fasta.contig Assembly.fasta
ln -s $ref Reference.fasta
echo EULER-SR>Assembler.txt
