source ../0parameters.sh
source ../0sim3-parameters.sh
mpirun -np $nproc Ray.0 -p $left $right $length $sd &> log
print-latex.sh $ref Ray-Contigs.fasta Ray.0
