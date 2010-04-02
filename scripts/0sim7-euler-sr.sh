source ../0parameters.sh
source ../0sim8-parameters.sh
cat $left $right > reads.fasta
Assemble.pl reads.fasta $wordSize &> log1
print-latex.sh $ref reads.fasta.contig EULER-SR
