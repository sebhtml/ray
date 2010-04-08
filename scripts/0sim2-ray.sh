source ../0sim2-parameters.sh
source ../0parameters.sh
mpirun $MPIOPTS -np $nproc Ray.0 -s $reads &> log
print-latex.sh $ref  Ray.0-Contigs.fasta Ray.0
