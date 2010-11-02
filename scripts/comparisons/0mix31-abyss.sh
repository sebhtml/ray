source ../0mix31-parameters.sh
source ../0parameters.sh
ABYSS -k$wordSize $r1 $r2 $r3 -o Assembly.fasta
echo ABySS>Assembler.txt
ln -s $ref Reference.fasta
