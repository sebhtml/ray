
source ../0parameters.sh
source ../0mix2-parameters.sh


time mpirun -np $nproc Ray -s $file1illumina -s $file2illumina &> log1
print-latex.sh $adp1 Ray-Contigs.fasta Ray
