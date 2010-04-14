source ../0parameters.sh
source ../0sim8-parameters.sh
ABYSS -k$wordSize $left $right -o Assembly.fasta
ln -s $ref Reference.fasta
echo ABYSS > Assembler.txt
