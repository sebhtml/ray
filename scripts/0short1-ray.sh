source ../0parameters.sh
source ../0short1-parameters.sh


time mpirun -np $nproc Ray -p $left $right $length $sd &> log1
print-latex.sh $syrin Ray-Contigs.fasta Ray
