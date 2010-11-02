source ../0parameters.sh
source ../0short1-parameters.sh
renameReads-for-euler.py "" $left > left.fastq
renameReads-for-euler.py "" $right > right.fastq
abyss-pe k=$wordSize n=10 name=syrin lib="lib1" lib1="left.fastq right.fastq" 
ln -s syrin-contigs.fa Assembly.fasta
ln -s $syrin Reference.fasta
echo ABySS > Assembler.txt
