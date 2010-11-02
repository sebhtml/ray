# Ray 0.1.0
# Open-MPI 1.4.3
# 30 CPUs (of 32)
# 128 GB memory

nohup /software/openmpi-1.4.3/bin/mpirun --mca btl ^sm -np 30 ../Ray \
 -s /home/boiseb01/nuccore/chr1-10.fa \
 -s /home/boiseb01/nuccore/chr1-1.fa \
 -s /home/boiseb01/nuccore/chr1-2.fa \
 -s /home/boiseb01/nuccore/chr1-3.fa \
 -s /home/boiseb01/nuccore/chr1-4.fa \
 -s /home/boiseb01/nuccore/chr1-5.fa \
 -s /home/boiseb01/nuccore/chr1-6.fa \
 -s /home/boiseb01/nuccore/chr1-7.fa \
 -s /home/boiseb01/nuccore/chr1-8.fa \
 -s /home/boiseb01/nuccore/chr1-9.fa \
 -o /home/boiseb01/nuccore/chr1-$(hostname).fasta > chr1-$(hostname).log &
