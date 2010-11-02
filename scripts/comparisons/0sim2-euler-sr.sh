source ../0sim2-parameters.sh
source ../0parameters.sh
cp $reads reads.fasta
Assemble.pl reads.fasta $wordSize 
echo EULER-SR>Assembler.txt
ln -s $ref Reference.fasta
ln -s reads.fasta.contig Assembly.fasta
