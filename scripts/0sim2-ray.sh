source ../0sim2-parameters.sh
source ../0parameters.sh
mpirun $MPIOPTS -np $nproc Ray -s $reads &> log
print-latex.sh $ref  Ray-Contigs.fasta Ray
