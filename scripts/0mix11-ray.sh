source ../0mix11-parameters.sh
source ../0parameters.sh
mpirun $MPIOPTS -np $nproc Ray.0 -s   $p1left -s $p1right -s $p2left -s $p2right &>log1
print-latex.sh $mg1655 Ray-Contigs.fasta Ray.0

