source ../0parameters.sh
source ../0sim3-parameters.sh
mpirun $MPIOPTS -np $nproc Ray.0 -p $left $right 
echo Ray>Assembler.txt
ln -s $ref Reference.fasta
ln -s Ray-Contigs.fasta Assembly.fasta
