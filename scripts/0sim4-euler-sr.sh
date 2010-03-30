source ../0sim4-parameters.sh
source ../0parameters.sh

cp $reads reads.fasta
Assemble.pl reads.fasta $wordSize &> log1
print-latex.sh $ref reads.fasta.contig EULER-SR
