mpirun -tag-output -np $(cat PROCESSES)  ~/git-clones/ray/code/Ray  \
-p /data/users/boiseb01/sra/SRA001125/sdata/SRR001665_1.fastq \
   /data/users/boiseb01/sra/SRA001125/sdata/SRR001665_2.fastq \
-p /data/users/boiseb01/sra/SRA001125/sdata/SRR001666_1.fastq \
   /data/users/boiseb01/sra/SRA001125/sdata/SRR001666_2.fastq \
-o $0 -show-memory-usage

ValidateGenomeAssembly.sh ~/nuccore/Ecoli-k12-mg1655.fasta $0.Contigs.fasta $0.Ray
