source ../0parameters.sh
source ../0mix11-parameters.sh
source ../0mix13-parameters.sh
mpirun -np $nproc Ray -p $p1left  $p1right $length1 $sd1 -p $p2left $p2right $length2 $sd2 -s $r4541 -s $r4542 -s $r4543 &> log1
print-latex.sh $ref Ray-Contigs.fasta Ray
