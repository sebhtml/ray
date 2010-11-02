source ../0parameters.sh
source ../0mix2-parameters.sh
ABYSS -k$wordSize $r4541 $r4542 $r4543 $r4544 -o Assembly.fasta
echo ABySS>Assembler.txt
ln -s $ref Reference.fasta
