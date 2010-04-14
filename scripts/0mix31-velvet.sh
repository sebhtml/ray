source ../0parameters.sh
source ../0mix31-parameters.sh
velveth velvet $wordSize -fasta -short $r1 $r2 $r3 
velvetg velvet -exp_cov auto -cov_cutoff auto 
echo Velvet>Assembler.txt
ln -s $ref Reference.fasta
ln -s velvet/contigs.fa Assembly.fasta
