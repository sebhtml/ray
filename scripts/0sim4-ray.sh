source ../0sim4-parameters.sh
source ../0parameters.sh
mpirun $MPIOPTS -np $nproc Ray.0 -s $reads &> log
print-latex.sh $ref Ray-Contigs.fasta Ray.0

