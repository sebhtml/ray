source ../0parameters.sh
source ../0short1-parameters.sh
ABYSS -k$wordSize $left $right -o contigs &> log1
print-latex.sh $ref contigs ABySS
