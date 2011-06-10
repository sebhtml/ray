mpirun -np $(cat PROCESSES) \
  ~/git-clones/ray/code/Ray \
-i /data/users/boiseb01/sra/SRA001125/sdata/interleaved1.fastq \
-i /data/users/boiseb01/sra/SRA001125/sdata/interleaved2.fastq \
-o $0

ValidateGenomeAssembly.sh ~/nuccore/Ecoli-k12-mg1655.fasta $0.Contigs.fasta $0.Ray
