mpirun -np $(cat PROCESSES)  ./build-compression/Ray  \
-p /home/boiseb01/nuccore/200xStreptococcus-pneumoniae-R6.fasta_fragments_1.fasta.bz2 \
   /home/boiseb01/nuccore/200xStreptococcus-pneumoniae-R6.fasta_fragments_2.fasta.bz2 \
-o $0

ValidateGenomeAssembly.sh ~/nuccore/Streptococcus-pneumoniae-R6.fasta $0.Contigs.fasta $0.Ray
