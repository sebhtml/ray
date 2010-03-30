source ../0parameters.sh
source ../0sim8-parameters.sh

ABYSS -k$wordSize $left $right -o contigs &> log1
print-latex.sh $ref contigs ABySS
