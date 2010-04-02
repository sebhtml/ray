source ../0parameters.sh
source ../0mix13-parameters.sh
cat $r4541 $r4542 $r4543 > reads.fasta
Assemble.pl reads.fasta $wordSize &> log1
print-latex.sh $ref reads.fasta.contig EULER-SR
