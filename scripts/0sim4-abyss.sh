source ../0sim4-parameters.sh
source ../0parameters.sh
ABYSS -k$wordSize $reads -o Assembly.fasta
echo ABySS>Assembler.txt
ln -s $ref Reference.fasta

