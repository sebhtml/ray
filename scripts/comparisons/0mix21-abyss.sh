source ../0parameters.sh
source ../0mix2-parameters.sh
ABYSS -k$wordSize $file1illumina $file2illumina -o Assembly.fasta
ln -s $ref Reference.fasta
echo ABySS>Assembler.txt

