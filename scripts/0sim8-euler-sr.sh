source ../0parameters.sh
source ../0sim8-parameters.sh

cat $left $right > reads.fasta
echo '"([^/]*)_([12])" CloneLength=250 CloneVar=0 Type=1      # Illumina mate-pair' > mates.info
Assemble.pl reads.fasta $wordSize -ruleFile mates.info &>log2
print-latex.sh $ref reads.fasta.contig EULER-SR
