source ../0parameters.sh
source ../0sim5-parameters.sh
mpirun -np $nproc Ray.0 -p $left $right $length $sd &> log1
print-latex.sh $ref Ray.0-Contigs.fasta Ray.0
