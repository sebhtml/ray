source ../0parameters.sh
source ../0short3-parameters.sh
mpirun -np $nproc Ray.0 -s $reads &> log1
print-latex.sh $ref Ray.0-Contigs.fasta Ray.0
