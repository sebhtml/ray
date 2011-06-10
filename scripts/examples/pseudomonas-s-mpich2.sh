mpirun -np $(cat PROCESSES)  ~/git-clones/ray/code/Ray  \
-s ~/nuccore/Pseud,200b,2x50b,50X_1.fasta \
-s ~/nuccore/Pseud,200b,2x50b,50X_2.fasta \
-o $0

ValidateGenomeAssembly.sh ~/nuccore/Pseudomonas-aeruginosa-PAO1,-complete-genome.fasta $0.Contigs.fasta $0.Ray
