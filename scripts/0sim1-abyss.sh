source ../0sim1-parameters.sh
source ../0parameters.sh

ABYSS -k$wordSize $reads -o contigs &> log1
print-latex.sh $ref contigs ABySS
