mpirun -tag-output -np $(cat PROCESSES) ~/Ray/trunk/code/Ray \
-p /home/boiseb01/nuccore/s_1.fasta \
   /home/boiseb01/nuccore/s_2.fasta \
-o $0

ValidateGenomeAssembly.sh ~/nuccore/Streptococcus-pneumoniae-R6.fasta $0.Contigs.fasta $0.Ray
