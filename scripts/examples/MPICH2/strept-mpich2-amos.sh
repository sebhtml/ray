rm -rf strept-mpich2.fasta
rm -rf *.afg

mpirun -np 31 ~/Ray/trunk/code/Ray \
-p /home/boiseb01/nuccore/200xStreptococcus-pneumoniae-R6.fasta_fragments_1.fasta \
   /home/boiseb01/nuccore/200xStreptococcus-pneumoniae-R6.fasta_fragments_2.fasta \
-o strept-mpich2 -a

~/Ray/trunk/scripts/print-latex.sh ~/nuccore/Streptococcus-pneumoniae-R6.fasta strept-mpich2.fasta Ray 
