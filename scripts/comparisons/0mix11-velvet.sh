source ../0mix11-parameters.sh
source ../0parameters.sh
velveth velvet $wordSize -short -fastq  $p1left $p1right $p2left $p2right 
velvetg velvet -exp_cov 109 -cov_cutoff 10 
echo Velvet>Assembler.txt
ln -s $mg1655 Reference.fasta
ln -s velvet/contigs.fa Assembly.fasta
