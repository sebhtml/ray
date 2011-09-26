mpiexec -np $NSLOTS  $RAY_GIT_PATH/Ray  \
-p Pseud,200b,2x50b,50X_1.fasta Pseud,200b,2x50b,50X_2.fasta \
-o $TEST_NAME

ValidateGenomeAssembly.sh Pseudomonas-aeruginosa-PAO1,-complete-genome.fasta $TEST_NAME/Contigs.fasta $TEST_NAME.Ray
