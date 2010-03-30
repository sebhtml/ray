source ../0parameters.sh
source ../0sim8-parameters.sh

mpirun -np 1 Ray -s $left -s $right &> log1
print-latex.sh $ref Ray-Contigs.fasta Ray
