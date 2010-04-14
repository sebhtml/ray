source ../0mix11-parameters.sh
source ../0parameters.sh
mpirun $MPIOPTS -np $nproc Ray.0 -p $p1left $p1right $length1 $sd1 -p $p2left $p2right $length2 $sd2 
ln -s Ray-Contigs.fasta Assembly.fasta
ln -s $mg1655 Reference.fasta
echo Ray>Assembler.txt
