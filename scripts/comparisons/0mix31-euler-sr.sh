source ../0parameters.sh
source ../0mix31-parameters.sh
cat $r1 $r2 $r3 > reads.fasta
Assemble.pl reads.fasta $wordSize 
ln -s $ref Reference.fasta
ln -s reads.fasta.contig Assembly.fasta
echo EULER-SR>Assembler.txt

