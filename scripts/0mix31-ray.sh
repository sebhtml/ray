source ../0mix31-parameters.sh
source ../0parameters.sh
mpirun -np $nproc Ray -s $r1 -s $r2 -s $r3 &> Log
print-latex.sh $ref Ray-Contigs.fasta Ray
