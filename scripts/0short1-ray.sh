source ../0parameters.sh
source ../0short1-parameters.sh
mpirun $MPIOPTS -np $nproc Ray.0 -p $left $right $length $sd 
ln -s $syrin Reference.fasta
ln -s Ray-Contigs.fasta Assembly.fasta
echo Ray > Assembler.txt
