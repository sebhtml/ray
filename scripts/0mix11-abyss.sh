source ../0mix11-parameters.sh
source ../0parameters.sh
ABYSS -k$wordSize $p1left $p1right $p2left $p2right -o Assembly.fasta
ln -s $ref Reference.fasta
echo $assembler > Assembler.txt

