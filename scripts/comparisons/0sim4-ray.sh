source ../0sim4-parameters.sh
source ../0parameters.sh
mpirun $MPIOPTS -np $nproc Ray.0 -s $reads 
echo Ray>Assembler.txt
ln -s $ref Reference.fasta
ln -s Ray-Contigs.fasta Assembly.fasta

