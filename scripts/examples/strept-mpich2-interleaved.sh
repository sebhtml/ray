mpirun -np $(cat PROCESSES) ~/Ray/trunk/code/Ray -i /home/boiseb01/nuccore/strept-interleaved.fasta \
-o $0

ValidateGenomeAssembly.sh ~/nuccore/Streptococcus-pneumoniae-R6.fasta $0.Contigs.fasta $0.Ray
