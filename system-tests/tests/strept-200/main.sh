mpiexec -np $NSLOTS $RAY_GIT_PATH/Ray  \
-p 200xStreptococcus-pneumoniae-R6.fasta_fragments_1.fasta \
   200xStreptococcus-pneumoniae-R6.fasta_fragments_2.fasta 200 0 \
-o $TEST_NAME

ValidateGenomeAssembly.sh Streptococcus-pneumoniae-R6.fasta $TEST_NAME/Contigs.fasta $TEST_NAME.Ray
