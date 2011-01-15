/software/mpich2-1.3/bin/mpirun -np $(cat PROCESSES) \
~/Ray/trunk/code/Ray \
-i /data/users/boiseb01/sra/SRA001125/sdata/interleaved1.fastq \
-i /data/users/boiseb01/sra/SRA001125/sdata/interleaved2.fastq \
-o ecoli3

~/Ray/trunk/scripts/print-latex.sh ~/nuccore/Ecoli-k12-mg1655.fasta ecoli3.fasta $0.Ray
