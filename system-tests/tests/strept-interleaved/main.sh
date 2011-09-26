mpiexec -np $NSLOTS  $RAY_GIT_PATH/Ray  \
-i strept-interleaved.fasta \
-o $TEST_NAME

ValidateGenomeAssembly.sh Streptococcus-pneumoniae-R6.fasta $TEST_NAME/Contigs.fasta $TEST_NAME.Ray
