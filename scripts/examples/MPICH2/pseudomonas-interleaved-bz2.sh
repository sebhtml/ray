/software/mpich2-1.3/bin/mpirun -np 31 \
~/Ray/trunk/code/Ray \
-i /home/boiseb01/nuccore/inter.fasta.bz2 \
-o Pseudomonas.fasta \
|& tee pseudomonas.log

ls -lh pseudomonas.log

~/Ray/trunk/scripts/print-latex.sh \
~/nuccore/Pseudomonas-aeruginosa-PAO1\,-complete-genome.fasta \
Pseudomonas.fasta \
RayTheMighty

pseudomonas-interleaved-bz2.sh
