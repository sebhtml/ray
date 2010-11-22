/software/openmpi-1.4.3/bin/mpirun -np 28 ~/Ray/trunk/code/Ray -p SRR001665_1.fastq SRR001665_2.fastq -p SRR001666_1.fastq SRR001666_2.fastq |& tee openmpi-1.4.3.log
ls -lh openmpi-1.4.3.log
coli-openmpi.sh
