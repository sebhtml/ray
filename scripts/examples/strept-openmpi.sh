/software/openmpi-1.4.3/bin/mpirun -np 28 \
~/Ray/trunk/code/Ray \
-p /home/boiseb01/nuccore/200xStreptococcus-pneumoniae-R6.fasta_fragments_1.fasta \
   /home/boiseb01/nuccore/200xStreptococcus-pneumoniae-R6.fasta_fragments_2.fasta \
 |& tee strept-openmpi.log

ls -lh strept-openmpi.log
~/Ray/trunk/scripts/print-latex.sh ~/nuccore/Streptococcus-pneumoniae-R6.fasta Ray-Contigs.fasta Ray
