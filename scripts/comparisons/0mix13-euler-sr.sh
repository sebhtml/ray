source ../0parameters.sh
source ../0mix13-parameters.sh
cat $r4541 $r4542 $r4543 > reads.fasta
Assemble.pl reads.fasta $wordSize 
ln -s $ref Reference.fasta
ln -s reads.fasta.contig Assembly.fasta
echo EULER-SR>Assembler.txt
