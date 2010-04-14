source ../0sim5-parameters.sh
source ../0parameters.sh
ABYSS -k$wordSize $left $right -o Assembly.fasta
echo ABySS>Assembler.txt
ln -s $ref Reference.fasta
