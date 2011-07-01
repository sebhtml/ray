mpirun -np $(cat PROCESSES)  ~/git-clones/ray/code/Ray  \
-p /home/boiseb01/nuccore/200xStreptococcus-pneumoniae-R6.fasta_fragments_1.fasta \
   /home/boiseb01/nuccore/200xStreptococcus-pneumoniae-R6.fasta_fragments_2.fasta 200 0 \
-o $0

ValidateGenomeAssembly.sh ~/nuccore/Streptococcus-pneumoniae-R6.fasta $0.Contigs.fasta $0.Ray
