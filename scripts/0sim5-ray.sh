source ../0parameters.sh
source ../0sim5-parameters.sh
mpirun $MPIOPTS -np $nproc Ray.0 -p $left $right $length $sd &> log1
print-latex.sh $ref Ray-Contigs.fasta Ray.0
