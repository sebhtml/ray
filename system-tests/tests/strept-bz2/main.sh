mpiexec -np $NSLOTS  $RAY_GIT_PATH/system-tests/builds/build-compression/Ray  \
-p 200xStreptococcus-pneumoniae-R6.fasta_fragments_1.fasta.bz2 \
   200xStreptococcus-pneumoniae-R6.fasta_fragments_2.fasta.bz2 \
-o $TEST_NAME

ValidateGenomeAssembly.sh Streptococcus-pneumoniae-R6.fasta $TEST_NAME/Contigs.fasta $TEST_NAME.Ray
