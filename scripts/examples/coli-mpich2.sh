/software/mpich2-1.3/bin/mpirun -np 28 \
~/Ray/trunk/code/Ray \
-p /data/users/sra/SRA001125/sdata/SRR001665_1.fastq \
   /data/users/sra/SRA001125/sdata/SRR001665_2.fastq \
-p /data/users/sra/SRA001125/sdata/SRR001666_1.fastq \
   /data/users/sra/SRA001125/sdata/SRR001666_2.fastq \
-o ecoli.fasta \
|& tee coli-mpich2.log

ls -lh coli-mpich2.log

~/Ray/trunk/scripts/print-latex.sh ~/nuccore/Ecoli-k12-mg1655.fasta ecoli.fasta
