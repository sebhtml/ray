mpirun -tag-output -np $NSLOTS $RAY_GIT_PATH/code/Ray  \
-p 200xStreptococcus-pneumoniae-R6.fasta_fragments_1.fasta \
   200xStreptococcus-pneumoniae-R6.fasta_fragments_2.fasta \
-o $TEST_NAME -show-memory-usage

ValidateGenomeAssembly.sh Streptococcus-pneumoniae-R6.fasta $TEST_NAME.Contigs.fasta $TEST_NAME.Ray
