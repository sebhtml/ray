mpirun -tag-output -np 31 ~/Ray/trunk/code/Ray \
-p /home/boiseb01/nuccore/Large-Ecoli/200_1.fastq /home/boiseb01/nuccore/Large-Ecoli/200_2.fastq -p /home/boiseb01/nuccore/Large-Ecoli/1000_1.fastq /home/boiseb01/nuccore/Large-Ecoli/1000_2.fastq -p /home/boiseb01/nuccore/Large-Ecoli/4000_1.fastq /home/boiseb01/nuccore/Large-Ecoli/4000_2.fastq -p /home/boiseb01/nuccore/Large-Ecoli/10000_1.fastq /home/boiseb01/nuccore/Large-Ecoli/10000_2.fastq 

print-latex.sh /home/boiseb01/nuccore/Ecoli-k12-mg1655.fasta  RayOutput.fasta Ray
