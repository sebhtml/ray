source ../0sim1-parameters.sh
source ../0parameters.sh
mpirun $MPIOPTS -np $nproc Ray.0 -s $reads  &> log1
print-latex.sh $ref Ray-Contigs.fasta Ray.0
