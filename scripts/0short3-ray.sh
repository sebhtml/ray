source ../0parameters.sh
source ../0short3-parameters.sh
mpirun -np 5 Ray -s $reads &> log1
print-latex.sh $ref Ray-Contigs.fasta Ray
