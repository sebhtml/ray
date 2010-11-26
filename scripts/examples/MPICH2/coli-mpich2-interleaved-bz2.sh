/software/mpich2-1.3/bin/mpirun -np 31 ~/Ray/trunk/code/Ray \
-i /data/users/boiseb01/sra/SRA001125/sdata/interleaved1.fastq.bz2 \
-i /data/users/boiseb01/sra/SRA001125/sdata/interleaved2.fastq.bz2 \
-o ecoli 

~/Ray/trunk/scripts/print-latex.sh ~/nuccore/Ecoli-k12-mg1655.fasta ecoli.fasta 

