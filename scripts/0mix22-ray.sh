source ../0parameters.sh
source ../0mix2-parameters.sh
mpirun -np $nproc Ray.0 -s $r4541 -s $r4542 -s $r4543 -s $r4544 &> log1
print-latex.sh $ref Ray.0-Contigs.fasta Ray.0
