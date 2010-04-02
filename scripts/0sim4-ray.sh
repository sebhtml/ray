source ../0sim4-parameters.sh
source ../0parameters.sh
mpirun -np $nproc Ray -s $reads &> log
print-latex.sh $ref Ray-Contigs.fasta Ray

