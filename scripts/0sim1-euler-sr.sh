source ../0sim1-parameters.sh
source ../0parameters.sh

ABYSS -k$wordSize $reads -o contigs &> log1
Assemble.pl $reads $wordSize &> log
print-latex.sh $ref reads.fasta.contig EULER-SR
