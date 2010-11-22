/software/openmpi-1.4.3/bin/mpirun -np 28 \
~/Ray/trunk/code/Ray \
-p /home/boiseb01/nuccore/200xStreptococcus-pneumoniae-R6.fasta_fragments_1.fasta.bz2 \
   /home/boiseb01/nuccore/200xStreptococcus-pneumoniae-R6.fasta_fragments_2.fasta.bz2 \
 |& tee strept-openmpi-bz2.log

ls -lh strept-openmpi-bz2.log
~/Ray/trunk/scripts/print-latex.sh ~/nuccore/Streptococcus-pneumoniae-R6.fasta Ray-Contigs.fasta Ray
strept-openmpi-bz2.sh
