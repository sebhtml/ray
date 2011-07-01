mpirun -tag-output -np $(cat PROCESSES) ./build-64/Ray \
-p ~/nuccore/PG2-dataset/pg2_1.fasta \
   ~/nuccore/PG2-dataset/pg2_1.fasta \
-k 70 \
-o $0

ValidateGenomeAssembly.sh ~/nuccore/Mycoplasma_agalactiae_PG2.fasta $0.Contigs.fasta $0.Ray
