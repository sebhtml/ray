mpirun -np $(cat PROCESSES) \
~/Ray/trunk/code/Ray \
-i /data/users/boiseb01/sra/SRA001125/sdata/interleaved1.fastq \
-i /data/users/boiseb01/sra/SRA001125/sdata/interleaved2.fastq \
-o $0

~/Ray/trunk/scripts/print-latex.sh ~/nuccore/Ecoli-k12-mg1655.fasta $0.fasta $0.Ray
