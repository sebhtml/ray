source ../0sim5-parameters.sh
source ../0parameters.sh

ABYSS -k$wordSize $left $right -o contigs &> log1
print-latex.sh $ref contigs ABySS
