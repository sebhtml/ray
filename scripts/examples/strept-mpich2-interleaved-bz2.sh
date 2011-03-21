mpirun -np $(cat PROCESSES) ~/Ray/trunk/code/Ray -i /home/boiseb01/nuccore/strept-interleaved.fasta.bz2 \
-o $0

~/Ray/trunk/scripts/print-latex.sh ~/nuccore/Streptococcus-pneumoniae-R6.fasta $0.fasta $0.Ray
