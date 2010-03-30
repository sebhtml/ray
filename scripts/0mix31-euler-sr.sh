source ../0parameters.sh
source ../0mix31-parameters.sh
cat $r1 $r2 $r3 > reads.fasta
Assemble.pl reads.fasta $wordSize &> log2
print-latex.sh $ref reads.fasta.contig EULER-SR

