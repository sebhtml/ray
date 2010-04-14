source ../0parameters.sh
source ../0short3-parameters.sh
ABYSS -k$wordSize $reads -o Assembly.fasta
ln -s $ref Reference.fasta
echo ABySS > Assembler.txt
