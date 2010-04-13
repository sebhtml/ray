source ../0parameters.sh
source ../0mix2-parameters.sh
mpirun $MPIOPTS -np $nproc Ray.0 -s $file1illumina -s $file2illumina -s $r4541 -s $r4542 -s $r4543 -s $r4544 &> log1
print-latex.sh $ref Ray-Contigs.fasta Ray.0
