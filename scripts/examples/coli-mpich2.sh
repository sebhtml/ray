/software/mpich2-1.3/bin/mpirun -np 28 \
~/Ray/trunk/code/Ray \
-p /data/users/sra/SRA001125/sdata/SRR001665_1.fastq.gz \
   /data/users/sra/SRA001125/sdata/SRR001665_2.fastq.gz \
-p /data/users/sra/SRA001125/sdata/SRR001666_1.fastq.gz \
   /data/users/sra/SRA001125/sdata/SRR001666_2.fastq.gz \
|& tee mpich2-1.3.log
ls -lh mpich2-1.3.log
