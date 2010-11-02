source ../0parameters.sh
source ../0short1-parameters.sh
ABYSS -k$wordSize $left $right -o Assembly.fasta 
ln -s $ref Reference.fasta
echo ABySS > Assembler.txt
