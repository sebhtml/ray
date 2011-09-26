mpiexec -np $NSLOTS  $RAY_GIT_PATH/system-tests/builds/build-compression/Ray  \
-i Pseudo-inter.fasta.bz2 -o $TEST_NAME

ValidateGenomeAssembly.sh Pseudomonas-aeruginosa-PAO1\,-complete-genome.fasta $TEST_NAME/Contigs.fasta $TEST_NAME.Ray
