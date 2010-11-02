source ../0sim8-parameters.sh
source ../0parameters.sh
abyss-pe k=$wordSize n=10 name=test lib="lib1" lib1="$left $right"
ln -s test-contigs.fa Assembly.fasta
ln -s $ref Reference.fasta
echo ABySS > Assembler.txt
