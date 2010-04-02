source ../0sim1-parameters.sh
source ../0parameters.sh
mpirun -np $nproc Ray -s $reads  &> log1
print-latex.sh $ref Ray-Contigs.fasta Ray
