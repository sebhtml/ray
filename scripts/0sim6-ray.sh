source ../0sim5-parameters.sh
source ../0parameters.sh
mpirun -np $nproc Ray.0 -s $left -s $right &> log1
print-latex.sh $ref Ray-Contigs.fasta Ray.0
