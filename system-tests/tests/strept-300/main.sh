mpirun -tag-output -np $NSLOTS $RAY_GIT_PATH/code/Ray \
-p s_1.fasta \
   s_2.fasta \
-o $TEST_NAME

ValidateGenomeAssembly.sh Streptococcus-pneumoniae-R6.fasta $TEST_NAME.Contigs.fasta $TEST_NAME.Ray
