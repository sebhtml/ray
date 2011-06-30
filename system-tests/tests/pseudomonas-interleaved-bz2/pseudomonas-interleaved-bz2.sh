mpirun -np $(cat PROCESSES)  ./build-compression/Ray  \
-i /home/boiseb01/nuccore/Pseudo-inter.fasta.bz2 -o $0

ValidateGenomeAssembly.sh \
~/nuccore/Pseudomonas-aeruginosa-PAO1\,-complete-genome.fasta $0.Contigs.fasta $0.Ray
