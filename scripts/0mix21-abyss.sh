
source ../0parameters.sh
source ../0mix2-parameters.sh

time ABYSS -k$wordSize $file1illumina $file2illumina -o abyss-contigs &>/dev/null
print-latex.sh $adp1 abyss-contigs ABySS

