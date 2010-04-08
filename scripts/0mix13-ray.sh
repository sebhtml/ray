source ../0parameters.sh
source ../0mix13-parameters.sh
mpirun -np $nproc Ray.0 -s $r4541 -s $r4542 -s $r4543 &> log1
print-latex.sh $ref Ray-Contigs.fasta  Ray.0
