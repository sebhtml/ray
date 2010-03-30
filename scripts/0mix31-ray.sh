

source ../0mix31-parameters.sh

mpirun -np 10 Ray -s $r1 -s $r2 -s $r3 &> Log
print-latex.sh $ref Ray-Contigs.fasta Ray
