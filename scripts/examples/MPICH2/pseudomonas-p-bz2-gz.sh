/software/mpich2-1.3/bin/mpirun -np 31 ~/Ray/trunk/code/Ray \
-p ~/nuccore/Pseud,200b,2x50b,50X_1.fasta ~/nuccore/Pseud,200b,2x50b,50X_2.fasta \
-o pseudo.fasta |& tee pseudomonas-p-mpich2.log

~/Ray/trunk/scripts/print-latex.sh \
~/nuccore/Pseudomonas-aeruginosa-PAO1,-complete-genome.fasta pseudo.fasta Ray | tee -a  pseudomonas-p-mpich2.log

