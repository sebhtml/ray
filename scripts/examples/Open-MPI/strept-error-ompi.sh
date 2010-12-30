rm -rf strept-ompi.fasta

/software/openmpi-1.4.3/bin/mpirun -np 16 ~/Ray/trunk/code/Ray \
-p /home/boiseb01/nuccore/exStreptococcus-pneumoniae-R6.fasta_fragments_1.fasta \
   /home/boiseb01/nuccore/exStreptococcus-pneumoniae-R6.fasta_fragments_2.fasta \
-o strept-ompi

~/Ray/trunk/scripts/print-latex.sh ~/nuccore/Streptococcus-pneumoniae-R6.fasta strept-ompi.fasta Ray 
