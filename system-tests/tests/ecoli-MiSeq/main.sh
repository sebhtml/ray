mpiexec -output-filename $TEST_NAME \
-n $NSLOTS $RAY_GIT_PATH/Ray \
-p MiSeq_Ecoli_MG1655_110527_R1.fastq \
   MiSeq_Ecoli_MG1655_110527_R2.fastq \
-o $TEST_NAME -k 31

ValidateGenomeAssembly.sh Ecoli-k12-mg1655.fasta $TEST_NAME/Contigs.fasta $TEST_NAME.Ray
