source ../0parameters.sh
source ../0short1-parameters.sh
mpirun $MPIOPTS -np $nproc Ray.0 -s $left -s $right  
ln -s Ray-Contigs.fasta Assembly.fasta
echo Ray > Assembler.txt
ln -s $syrin Reference.fasta
