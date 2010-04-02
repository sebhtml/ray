source ../0parameters.sh
source ../0mix32-parameters.sh
mpirun -np $nproc Ray -s $r4541 -s $r4542 -s $r4543 -s $r4544 -s $r4545 &> log1
print-latex.sh $ref Ray-Contigs.fasta Ray
