
. 0parameters.sh
. 0mix2-parameters.sh


time mpirun -np $nproc Ray -s $file1illumina -s $file2illumina &>/dev/null
print-latex.sh $adp1 Ray-Contigs.fasta Ray
