source ../0parameters.sh
source ../0mix2-parameters.sh
ABYSS -k$wordSize $file1illumina $file2illumina $r4541 $r4542 $r4543 $r4544 -o contigs &> log1
print-latex.sh $ref contigs ABySS
