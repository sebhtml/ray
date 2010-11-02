source ../0sim1-parameters.sh
source ../0parameters.sh
ABYSS -k$wordSize $reads -o Assembly.fasta
ln -s $ref Reference.fasta
echo ABySS>Assembler.txt
