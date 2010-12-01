/software/mpich2-1.3/bin/mpirun -np 31 ~/Ray/trunk/code/Ray \
-p /home/boiseb01/nuccore/200xStreptococcus-pneumoniae-R6.fasta_fragments_1.fasta \
   /home/boiseb01/nuccore/200xStreptococcus-pneumoniae-R6.fasta_fragments_2.fasta 200 0 \
-o strept-mpich-200

~/Ray/trunk/scripts/print-latex.sh ~/nuccore/Streptococcus-pneumoniae-R6.fasta strept-mpich-200.fasta Ray 
