source ../0mix11-parameters.sh
source ../0parameters.sh
abyss-pe k=$wordSize n=50 name=mg1655 lib="lib1 lib2" lib1="$p1left $p1right" lib2="$p2left $p2right" 
ln -s mg1655-contigs.fa Assembly.fasta
echo ABySS>Assembler.txt
ln -s $ref Reference.fasta
