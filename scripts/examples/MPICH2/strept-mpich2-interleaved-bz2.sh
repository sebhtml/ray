/software/mpich2-1.3/bin/mpirun -np $(cat PROCESSES) ~/Ray/trunk/code/Ray -i /home/boiseb01/nuccore/strept-interleaved.fasta.bz2 

~/Ray/trunk/scripts/print-latex.sh ~/nuccore/Streptococcus-pneumoniae-R6.fasta RayOutput.fasta $0.Ray
