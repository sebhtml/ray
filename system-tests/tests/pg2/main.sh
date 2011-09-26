mpiexec -tag-output -np $NSLOTS $RAY_GIT_PATH/system-tests/builds/build-64/Ray \
-p pg2_1.fasta \
   pg2_2.fasta \
-k 70 \
-o $TEST_NAME 

ValidateGenomeAssembly.sh Mycoplasma_agalactiae_PG2.fasta $TEST_NAME/Contigs.fasta $TEST_NAME.Ray
