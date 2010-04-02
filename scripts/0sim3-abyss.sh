source ../0parameters.sh
source ../0sim3-parameters.sh
abyss-pe k=$wordSize n=10 name=test lib="lib1" lib1="$left $right" &> log
print-latex.sh $ref test-contigs.fa ABySS
