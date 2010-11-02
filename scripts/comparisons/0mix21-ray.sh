source ../0parameters.sh
source ../0mix2-parameters.sh
mpirun $MPIOPTS -np $nproc Ray.0 -s $file1illumina -s $file2illumina
ln -s Ray-Contigs.fasta Assembly.fasta
ln -s $ref  Reference.fasta
echo Ray>Assembler.txt
