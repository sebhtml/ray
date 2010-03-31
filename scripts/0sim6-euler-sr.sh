source ../0sim5-parameters.sh
source ../0parameters.sh
cat $left $right > reads.fasta
Assemble.pl reads.fasta $wordSize &> log
print-latex.sh $ref reads.fasta.contig EULER-SR
