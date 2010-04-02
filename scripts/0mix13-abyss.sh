source ../0parameters.sh
source ../0mix13-parameters.sh
ABYSS -k$wordSize $r4541 $r4542 $r4543 -o contigs &> log1
print-latex.sh $ref contigs ABySS
