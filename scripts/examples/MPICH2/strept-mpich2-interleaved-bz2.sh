/software/mpich2-1.3/bin/mpirun -np 31 ~/Ray/trunk/code/Ray \
-i /home/boiseb01/nuccore/strept-interleaved.fasta.bz2 \
 |& tee strept-mpich2-interleaved-bz2.log

~/Ray/trunk/scripts/print-latex.sh ~/nuccore/Streptococcus-pneumoniae-R6.fasta RayOutput.fasta Ray |& tee -a strept-mpich2-interleaved-bz2.log
