source ../0sim4-parameters.sh
source ../0parameters.sh
ABYSS -k$wordSize $reads -o contigs &> log
print-latex.sh $ref contigs ABySS

