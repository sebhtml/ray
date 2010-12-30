/software/mpich2-1.3/bin/mpirun -np 31 ~/Ray/trunk/code/Ray \
-p /home/boiseb01/nuccore/Left-errors.fasta \
   /home/boiseb01/nuccore/Right-errors.fasta \
-o strept-mpich2

~/Ray/trunk/scripts/print-latex.sh ~/nuccore/Streptococcus-pneumoniae-R6.fasta strept-mpich2.fasta Ray 
