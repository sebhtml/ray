source ../0sim2-parameters.sh
source ../0parameters.sh
velveth velvet $wordSize -fasta -short $reads 
velvetg velvet -cov_cutoff auto -exp_cov auto 
echo Velvet>Assembler.txt
ln -s $ref Reference.fasta
ln -s velvet/contigs.fa Assembly.fasta
