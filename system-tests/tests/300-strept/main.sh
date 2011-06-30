
mpirun -tag-output -np $NSLOT ~/git-clones/ray/code/Ray \
-p s_1.fasta \
   s_2.fasta \
-o $RayTestName

ValidateGenomeAssembly.sh Streptococcus-pneumoniae-R6.fasta $RayTestName.Contigs.fasta $RayTestName.Ray
