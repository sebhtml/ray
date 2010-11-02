source ../0sim2-parameters.sh
source ../0parameters.sh
ABYSS -k$wordSize $reads -o Assembly.fasta
echo ABySS>Assembler.txt
ln -s $ref Reference.fasta
