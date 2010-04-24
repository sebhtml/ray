to compile:

 make

to run on paired reads:

 mpirun -np 25 Ray -p lib1left.fasta lib1right.fasta -p lib2left.fasta lib2right.fasta -o CONTIGS.fasta

to output amos file:

 mpirun -np 25 Ray -s 1.fasta -p 21.fasta 22.fasta -a AMOS.amos -o MyContigs.fasta


see http://denovoassembler.sf.net/ or Wiki.txt for complete information.
