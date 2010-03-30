source ../0sim4-parameters.sh
source ../0parameters.sh


mpirun -np 1 Ray -s $reads &> log
print-latex $ref Ray-Contigs.fasta Ray

