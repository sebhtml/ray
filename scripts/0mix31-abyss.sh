source ../0mix31-parameters.sh
source ../0parameters.sh
ABYSS -k$wordSize $r1 $r2 $r3 -o contigs &> log1
print-latex.sh $ref contigs ABySS
