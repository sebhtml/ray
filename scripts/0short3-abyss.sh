source ../0parameters.sh
source ../0short3-parameters.sh
ABYSS -k$wordSize $reads -o contigs &> log1
print-latex.sh $ref contigs ABySS
