source ../0mix31-parameters.sh
source ../0parameters.sh
mpirun $MPIOPTS -np $nproc Ray.0 -s $r1 -s $r2 -s $r3 
ln -s Ray-Contigs.fasta Assembly.fasta
ln -s $ref Reference.fasta
echo Ray>Assembler.txt
