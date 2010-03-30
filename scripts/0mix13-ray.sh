source ../0parameters.sh
source ../0mix13-parameters.sh

mpirun -np 1 Ray -s $r4541 -s $r4542 -s $r4543 &> log1
print-latex.sh $ref Ray-Contigs.fasta  Ray
