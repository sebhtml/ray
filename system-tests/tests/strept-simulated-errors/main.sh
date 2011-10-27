mpiexec -output-filename $TEST_NAME \
-np $NSLOTS $RAY_GIT_PATH/Ray  \
-p strept_200_1.fastq \
   strept_200_2.fastq \
-o $TEST_NAME

ValidateGenomeAssembly.sh Streptococcus-pneumoniae-R6.fasta $TEST_NAME/Contigs.fasta $TEST_NAME.Ray
