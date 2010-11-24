/software/mpich2-1.3/bin/mpirun -np 31 ~/Ray/trunk/code/Ray \
-i /home/boiseb01/nuccore/inter.fasta.bz2 -o Pseudomonas 

~/Ray/trunk/scripts/print-latex.sh \
~/nuccore/Pseudomonas-aeruginosa-PAO1\,-complete-genome.fasta Pseudomonas.fasta RayTheMighty
