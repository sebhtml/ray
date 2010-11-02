source ../0mix11-parameters.sh
source ../0parameters.sh
mpirun $MPIOPTS -np $nproc Ray.0 -s   $p1left -s $p1right -s $p2left -s $p2right 
ln -s Ray-Contigs.fasta Assembly.fasta
ln -s $mg1655 Reference.fasta
echo Ray>Assembler.txt
