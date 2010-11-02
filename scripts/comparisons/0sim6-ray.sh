source ../0sim5-parameters.sh
source ../0parameters.sh
mpirun $MPIOPTS -np $nproc Ray.0 -s $left -s $right 
ln -s Ray-Contigs.fasta Assembly.fasta
ln -s $ref Reference.fasta
echo Ray>Assembler.txt
