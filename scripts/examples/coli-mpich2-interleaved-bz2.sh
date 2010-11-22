/software/mpich2-1.3/bin/mpirun -np 31 \
~/Ray/trunk/code/Ray \
-i /data/users/sra/SRA001125/sdata/interleaved1.fastq.bz2 \
-i /data/users/sra/SRA001125/sdata/interleaved2.fastq.bz2 \
-o ecoli.fasta \
|& tee mpich2-1.3.log

ls -lh mpich2-1.3.log

~/Ray/trunk/scripts/print-latex.sh ~/nuccore/Ecoli-k12-mg1655.fasta ecoli.fasta
