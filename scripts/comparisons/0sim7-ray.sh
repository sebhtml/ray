source ../0parameters.sh
source ../0sim8-parameters.sh
mpirun $MPIOPTS -np $nproc Ray.0 -s $left -s $right
ln -s Ray-Contigs.fasta Assembly.fasta
echo Ray > Assembler.txt
ln -s $ref Reference.fasta
