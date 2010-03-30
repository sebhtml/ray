source ../0parameters.sh
source ../0short1-parameters.sh

time abyss-pe k=$wordSize n=5 name=syrin lib="lib1" lib1="$left $right" &>log1
print-latex.sh $syrin syrin-contigs.fa ABySS
