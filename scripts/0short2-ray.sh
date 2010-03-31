source ../0parameters.sh
source ../0short1-parameters.sh
mpirun -np 5 Ray -s $left -s $right  &> log1
print-latex.sh $syrin Ray-Contigs.fasta Ray
