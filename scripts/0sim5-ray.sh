source ../0parameters.sh
source ../0sim5-parameters.sh
mpirun $MPIOPTS -np $nproc Ray.0 -p $left $right $length $sd 
ln -s Ray-Contigs.fasta Assembly.fasta
ln -s $ref Reference.fasta
echo Ray>Assembler.txt
