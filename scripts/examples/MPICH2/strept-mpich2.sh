rm -rf strept-mpich2.fasta

/software/mpich2-1.3/bin/mpirun -np 16 ~/Ray/trunk/code/Ray \
-p /home/boiseb01/nuccore/200xStreptococcus-pneumoniae-R6.fasta_fragments_1.fasta \
   /home/boiseb01/nuccore/200xStreptococcus-pneumoniae-R6.fasta_fragments_2.fasta \
-o strept-mpich2

~/Ray/trunk/scripts/print-latex.sh ~/nuccore/Streptococcus-pneumoniae-R6.fasta strept-mpich2.fasta Ray 
