source ../0parameters.sh
source ../0short1-parameters.sh


time mpirun -np $nproc Ray -p $left $right $length $sd &> /dev/null
print-latex.sh $syrin Ray-Contigs.fasta Ray
