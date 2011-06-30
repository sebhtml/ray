mpirun -tag-output -np $(cat PROCESSES)  ~/git-clones/ray/code/Ray  \
-debug-bubbles \
 -p ~/RECOMB-seq-2011/L1_1.fasta ~/RECOMB-seq-2011/L1_2.fasta -p ~/RECOMB-seq-2011/L2_1.fasta ~/RECOMB-seq-2011/L2_2.fasta \
 -p ~/RECOMB-seq-2011/L3_1.fasta ~/RECOMB-seq-2011/L3_2.fasta -o $0

ValidateGenomeAssembly.sh ~/nuccore/Ecoli-k12-mg1655.fasta $0.Contigs.fasta $0.Ray

