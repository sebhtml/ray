mpirun -tag-output -np $NSLOTS  ~/git-clones/ray/code/Ray  \
-p SRR001665_1.fastq \
   SRR001665_2.fastq \
-p SRR001666_1.fastq \
   SRR001666_2.fastq \
-o $RayTestName -show-memory-usage

ValidateGenomeAssembly.sh Ecoli-k12-mg1655.fasta $RayTestName.Contigs.fasta $RayTestName.Ray
