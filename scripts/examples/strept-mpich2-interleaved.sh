mpirun -np $(cat PROCESSES) ~/Ray/trunk/code/Ray -i /home/boiseb01/nuccore/strept-interleaved.fasta \
-o $0

~/Ray/trunk/scripts/print-latex.sh ~/nuccore/Streptococcus-pneumoniae-R6.fasta $0.Contigs.fasta $0.Ray
