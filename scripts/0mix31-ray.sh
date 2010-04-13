source ../0mix31-parameters.sh
source ../0parameters.sh
mpirun $MPIOPTS -np $nproc Ray.0 -s $r1 -s $r2 -s $r3 &> Log
print-latex.sh $ref Ray-Contigs.fasta Ray.0
