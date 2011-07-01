mpirun -np $NSLOTS \
  ~/git-clones/ray/code/Ray \
-i interleaved1.fastq \
-i interleaved2.fastq \
-o $RayTestName

ValidateGenomeAssembly.sh Ecoli-k12-mg1655.fasta $RayTestName.Contigs.fasta $RayTestName.Ray
