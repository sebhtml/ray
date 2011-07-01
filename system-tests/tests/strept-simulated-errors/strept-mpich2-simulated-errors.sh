mpirun -tag-output -np $(cat PROCESSES)   ~/git-clones/ray/code/Ray  \
-p /home/boiseb01/nuccore/strept_200_1.fastq \
   /home/boiseb01/nuccore/strept_200_2.fastq \
-o $0 -show-memory-usage

ValidateGenomeAssembly.sh ~/nuccore/Streptococcus-pneumoniae-R6.fasta $0.Contigs.fasta $0.Ray
