mpirun -np $(cat PROCESSES) ~/Ray/trunk/code/Ray \
-i /home/boiseb01/nuccore/inter.fasta.bz2 -o $0

~/Ray/trunk/scripts/print-latex.sh \
~/nuccore/Pseudomonas-aeruginosa-PAO1\,-complete-genome.fasta $0.fasta $0.Ray
