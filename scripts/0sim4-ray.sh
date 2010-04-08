source ../0sim4-parameters.sh
source ../0parameters.sh
mpirun -np $nproc Ray.0 -s $reads &> log
print-latex.sh $ref Ray.0-Contigs.fasta Ray.0

