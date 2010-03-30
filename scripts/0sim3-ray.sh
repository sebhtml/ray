source ../0parameters.sh
source ../0sim3-parameters.sh

mpirun -np 1 Ray -p $left $right $length $sd &> log
print-latex.sh $ref Ray-Contigs.fasta Ray
