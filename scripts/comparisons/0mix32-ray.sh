source ../0parameters.sh
source ../0mix32-parameters.sh
mpirun $MPIOPTS -np $nproc Ray.0 -s $r4541 -s $r4542 -s $r4543 -s $r4544 -s $r4545
ln -s Ray-Contigs.fasta Assembly.fasta
ln -s $ref Reference.fasta
echo Ray>Assembler.txt
