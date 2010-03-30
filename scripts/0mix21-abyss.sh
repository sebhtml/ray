
source ../0parameters.sh
source ../0mix2-parameters.sh

time ABYSS -k$wordSize $file1illumina $file2illumina -o abyss-contigs &> log1
print-latex.sh $adp1 abyss-contigs ABySS

