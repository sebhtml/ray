mpirun -np $(cat PROCESSES)  ./build-compression/Ray  -i /home/boiseb01/nuccore/strept-interleaved.fasta.bz2 \
-o $0

ValidateGenomeAssembly.sh ~/nuccore/Streptococcus-pneumoniae-R6.fasta $0.Contigs.fasta $0.Ray
