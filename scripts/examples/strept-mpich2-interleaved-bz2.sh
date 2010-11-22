/software/mpich2-1.3/bin/mpirun -np 31 \
~/Ray/trunk/code/Ray \
-i /home/boiseb01/nuccore/strept-interleaved.fasta.bz2 \
 |& tee log

ls -lh log
~/Ray/trunk/scripts/print-latex.sh ~/nuccore/Streptococcus-pneumoniae-R6.fasta Ray-Contigs.fasta Ray
