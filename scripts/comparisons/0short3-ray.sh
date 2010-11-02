source ../0parameters.sh
source ../0short3-parameters.sh
mpirun $MPIOPTS -np $nproc Ray.0 -s $reads
ln -s  Ray-Contigs.fasta Assembly.fasta
ln -s $ref Reference.fasta
echo Ray > Assembler.txt
