
/software/mpich2-1.3/bin/mpirun -np 31 ~/Ray/trunk/code/Ray \
-p /data/users/boiseb01/sra/SRA001125/sdata/SRR001665_1.fastq.gz \
   /data/users/boiseb01/sra/SRA001125/sdata/SRR001665_2.fastq.gz \
-p /data/users/boiseb01/sra/SRA001125/sdata/SRR001666_1.fastq.gz \
   /data/users/boiseb01/sra/SRA001125/sdata/SRR001666_2.fastq.gz \
-o ecoli1 -a

~/Ray/trunk/scripts/print-latex.sh ~/nuccore/Ecoli-k12-mg1655.fasta ecoli1.fasta Ray_THEONE
