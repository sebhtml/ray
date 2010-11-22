/software/openmpi-1.4.3/bin/mpirun --mca btl ^sm -np 31 ./code/Ray \
-p ~/nuccore/Pseud,200b,2x50b,50X_1.fasta ~/nuccore/Pseud,200b,2x50b,50X_2.fasta \
-o pseudo.fasta |& tee pseudomonas-p.log

~/Ray/trunk/scripts/print-latex.sh \
~/nuccore/Pseudomonas-aeruginosa-PAO1,-complete-genome.fasta pseudo.fasta Ray | tee -a  pseudomonas-p.log
