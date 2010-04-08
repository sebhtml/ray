source ../0parameters.sh
source ../0sim8-parameters.sh
mpirun -np $nproc Ray.0 -s $left -s $right &> log1
print-latex.sh $ref Ray.0-Contigs.fasta Ray.0
