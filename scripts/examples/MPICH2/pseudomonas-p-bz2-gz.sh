/software/mpich2-1.3/bin/mpirun -np 31 ~/Ray/trunk/code/Ray \
-p ~/nuccore/Pseud,200b,2x50b,50X_1.fasta.bz2 ~/nuccore/Pseud,200b,2x50b,50X_2.fasta.gz \
-o pseudo 

~/Ray/trunk/scripts/print-latex.sh \
~/nuccore/Pseudomonas-aeruginosa-PAO1,-complete-genome.fasta pseudo.fasta Ray 

