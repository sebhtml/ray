mpirun -np $(cat PROCESSES)  ./build-compression/Ray  \
-p ~/nuccore/Pseud,200b,2x50b,50X_1.fasta.bz2 ~/nuccore/Pseud,200b,2x50b,50X_2.fasta.gz \
-o $0

ValidateGenomeAssembly.sh \
~/nuccore/Pseudomonas-aeruginosa-PAO1,-complete-genome.fasta $0.Contigs.fasta $0.Ray
